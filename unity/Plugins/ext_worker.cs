using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using UnityEngine;
using Random = System.Random;

public class ext_worker
{
   [DllImport("ext_worker")]
   public static extern bool start_tcworker(string tcpath, string name);

   [DllImport("ext_worker")]
   public static extern void terminate_tcworker(string name);

   [DllImport("ext_worker")]
   public static extern bool send_command(string name, string cmd);

   [DllImport("ext_worker", EntryPoint = "read")]
   private static extern bool read_internal(string name, out IntPtr data);

   public static bool read(string name, out string msg)
   {
      msg = String.Empty;

      if (read_internal(name, out var ptr))
      {
         if (ptr != IntPtr.Zero)
            msg = Marshal.PtrToStringAnsi(ptr);
         return true;
      }

      return false;
   }
}

public class ExternalWorkerManager
{
   public struct Worker
   {
      public WorkerType WorkerType; //todo: enum it
      public string WorkerName; //unique per worker, used for ipc
   }
   
   public struct Workload
   {
      public WorkerType WorkerType; //todo: enum it
      public string Cmd;
      public int Priority; //todo
      public Action Callback; //todo
      public JobStatus Status;
   }

   public enum WorkerType
   {
      TextureCompressorForeground,
      TextureCompressorBackground,
   }

   public enum JobStatus
   {
      Pending,
      Ongoing,
      Finished,
      Failed
   }
   
   public class ExternalWorkerException : Exception {}

   private static ExternalWorkerManager m_instance;
   private static object m_lock = new object();
   private const string m_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
   private const string m_tcHeartbeatMagicMessage = "202405230000heartbeat";
   private const int m_dispatchCheckIntervalInMs = 500;

   private static Dictionary<WorkerType, string> m_executablePaths = new Dictionary<WorkerType, string>();
   
   private volatile bool m_initialized = false;
   private volatile bool m_terminiateDispatch = false;
   private Thread m_jobDispatchThread;
   private volatile Queue<Workload> m_jobs = new Queue<Workload>();
   private volatile Dictionary<Worker, bool> m_workers = new Dictionary<Worker, bool>();

   public static ExternalWorkerManager GetInstance()
   {
      if (m_instance == null)
      {
         lock (m_lock)
         {
            if (m_instance == null)
               m_instance = new ExternalWorkerManager();
         }
      }

      return m_instance;
   }

   public static void SetTextureCompressorExecutablePath(string tcExecutablePath)
   {
      m_executablePaths[WorkerType.TextureCompressorForeground] = tcExecutablePath;
   }

   public void Prepare()
   {
      
   }

   public void AddJob(Workload job)
   {
      var addJobThread = new Thread((o =>
      {
         Workload workload = (Workload) o;
         lock (m_jobs)
         {
            workload.Status = JobStatus.Pending;
            m_jobs.Enqueue(workload);
         }
      }));
      addJobThread.Start(job);
   }

   private ExternalWorkerManager()
   {
      Initialize();
      Dispatch();
      
#if UNITY_EDITOR
      UnityEditor.EditorApplication.playModeStateChanged += (change =>
      {
         if (change == UnityEditor.PlayModeStateChange.EnteredEditMode)
         {
            Cleanup();
         }
      });
#else
            Application.quitting -= Cleanup;
            Application.quitting += Cleanup;
#endif
   }

   private void Initialize()
   {
      if (m_initialized)
         return;
      m_terminiateDispatch = false;
      // launch external worker process at start-up 
      var initializeExtWorkerThread = new Thread(() =>
      {
         lock (m_lock)
         {
            string errlog, errlog2 = String.Empty;
            if (StartWorker(WorkerType.TextureCompressorForeground, true, out errlog) &&
                StartWorker(WorkerType.TextureCompressorForeground, true, out errlog2))
            {
               m_initialized = true;
            }
            else
            {
               //print errlogs
               Debug.LogError(errlog);
               Debug.LogError(errlog2);
               //cleanup
               foreach (var kvp in m_workers)
               {
                  ext_worker.terminate_tcworker(kvp.Key.WorkerName);
               }
               m_workers.Clear();
               m_initialized = false;
            } 
         }
      });
      initializeExtWorkerThread.Start();
   }

   private void Cleanup()
   {
      // m_jobDispatchThread.Interrupt();
      m_terminiateDispatch = true;
      lock (m_workers)
      {
         foreach (var kvp in m_workers)
         {
            ext_worker.terminate_tcworker(kvp.Key.WorkerName);
         }
         m_workers.Clear();
      }

      lock (m_jobs)
      {
         m_jobs.Clear();
      }
   }

   private void Dispatch()
   {
      if (m_jobDispatchThread != null)
         return;
      m_jobDispatchThread = new Thread(() =>
      {
         while (true)
         {
            if (m_terminiateDispatch)
               break;
            
            if (!m_initialized)
            {
               //wait for initializing
               goto Suspend;
            }

            lock (m_workers)
            {
               if (m_workers.Count == 0)
               {
                  //maybe all the workers are rebooting?
                  //So just wait
                  goto Suspend;
               }
            }
            
            Workload currentJob;
            lock (m_jobs)
            {
               if (m_jobs.Count == 0)
               {
                  goto Suspend;
               }
               currentJob = m_jobs.Dequeue();
            }
            //find an available job worker
            bool found = false;
            lock (m_workers)
            {
               foreach (var kvp in m_workers)
               {
                  if (kvp.Value && kvp.Key.WorkerType == currentJob.WorkerType)
                  {
                     m_workers[kvp.Key] = false;
                     found = true;
                     var workingThread = new Thread(() => Work(kvp.Key, currentJob));
                     workingThread.Start();
                     break;
                  }
               }
            }
            if (!found)
            {
               lock (m_jobs)
               {
                  //put it back to the end of the queue
                  m_jobs.Enqueue(currentJob);
               }
            }
            continue;
Suspend:
            Thread.Sleep(m_dispatchCheckIntervalInMs);
         }
      });
      m_jobDispatchThread.Start();
   }

   private bool StartWorker(WorkerType type, bool retry, out string errlog)
   {
      int maxRetryCount = 10;
      int retryTimes = 1;
      bool result = true;
      string reason = String.Empty;
      errlog = String.Empty;
      do
      {
         string ipcName = "yatc-" + GenerateRandomIPCName(8);
         if (!ext_worker.start_tcworker(m_executablePaths[type], ipcName))
         {
            retryTimes++;
            result = false;
            reason = "Failed to launch external process at " + m_executablePaths[type];
            errlog += "[StartWorker]Error when starting tc at [" + retryTimes + "] time(s): " + reason + "\n";
            continue;
         }
         //send and receive 202405230000heartbeat
         if (!ext_worker.send_command(ipcName, m_tcHeartbeatMagicMessage))
         {
            retryTimes++;
            result = false;
            reason = "Failed to send heartbeat command via IPC#" + ipcName;
            errlog += "[StartWorker]Error when starting tc at [" + retryTimes + "] time(s): " + reason + "\n";
            //terminate the ext process
            ext_worker.terminate_tcworker(ipcName);
            continue;
         }

         if (!ext_worker.read(ipcName, out string msg))
         {
            retryTimes++;
            result = false;
            reason = "Failed to receive desired heartbeat message via IPC#" + ipcName;
            errlog += "[StartWorker]Error when starting tc at [" + retryTimes + "] time(s): " + reason + "\n";

            //terminate the ext process
            ext_worker.terminate_tcworker(ipcName);
            continue;
         }

         if (msg != m_tcHeartbeatMagicMessage)
         {
            retryTimes++;
            result = false;
            reason = "Heartbeat message mismatches, probably because of invalid worker version.";
            errlog += "[StartWorker]Error when starting tc at [" + retryTimes + "] time(s): " + reason + "\n";

            //terminate the ext process
            ext_worker.terminate_tcworker(ipcName);
            continue;
         }

         result = true;
         reason = String.Empty;

         lock (m_workers)
         {
            m_workers.Add(new Worker(){WorkerName = ipcName, WorkerType = 0}, true);
         }
      } while (!result && retry && (retryTimes <= maxRetryCount));

      return result;
   }

   private string GenerateRandomIPCName(int len)
   {
      Random random = new Random();
      StringBuilder sb = new StringBuilder(len);
        
      for (int i = 0; i < len; i++)
      {
         sb.Append(m_chars[random.Next(m_chars.Length)]);
      }
        
      return sb.ToString();
   }

   private void Work(Worker worker, Workload workload)
   {
      try
      {
         workload.Status = JobStatus.Ongoing;
         //Send cmd and handle recv here
         if (!ext_worker.send_command(worker.WorkerName, workload.Cmd))
            throw new ExternalWorkerException();
         if (!ext_worker.read(worker.WorkerName, out string msg))
            throw new ExternalWorkerException();
         
         if (msg == "Job done.")
         {
            workload.Status = JobStatus.Finished;
            workload.Callback?.Invoke();
            Debug.Log(msg);
         }
         else
         {
            Debug.LogError($"Error: Worker '{worker.WorkerName}' finished job with message: {msg}");
         }
      }
      catch (ExternalWorkerException _)
      {
         HandleExtWorkerException(worker, workload);
      }
      finally
      {
         lock (m_workers)
         {
            if (m_workers.TryGetValue(worker, out bool _))
               m_workers[worker] = true;
         }
      }
   }

   private void HandleExtWorkerException(Worker worker, Workload workload)
   {
      //connection closed, external process may be crashed
      //terminate this worker and create a new one
      lock (m_workers)
      {
         ext_worker.terminate_tcworker(worker.WorkerName);
         m_workers.Remove(worker);
      }

      StartWorker(worker.WorkerType, true, out string errlog);

      //enqueue the current job back to job queue
      lock (m_jobs)
      {
         workload.Status = JobStatus.Pending;
         m_jobs.Enqueue(workload);
      }
   }
}
