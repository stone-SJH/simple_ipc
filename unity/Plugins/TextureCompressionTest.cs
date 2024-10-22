using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using UnityEngine;

public class TextureCompressionTest : MonoBehaviour
{
#if XQ_DEBUG
    private const string tcpath = "E:/202405/ipc/TextureCompressionV1/bin/YaTCompress.exe";

    private const string cmd1 =
        "tc bc 1 E:\\202405\\ipc\\TextureCompressionV1\\bin\\input.png E:\\202405\\ipc\\TextureCompressionV1\\bin\\output1.dds -mipmap";
    private const string cmd2 =
        "tc bc 3 E:\\202405\\ipc\\TextureCompressionV1\\bin\\input.png E:\\202405\\ipc\\TextureCompressionV1\\bin\\output2.dds -mipmap";
    private const string cmd3 =
        "tc bc 7 E:\\202405\\ipc\\TextureCompressionV1\\bin\\input.png E:\\202405\\ipc\\TextureCompressionV1\\bin\\output3.dds -mipmap";
    private const string cmd5 =
        "tc astc 4x4 E:\\202405\\ipc\\TextureCompressionV1\\bin\\input.png E:\\202405\\ipc\\TextureCompressionV1\\bin\\output5.astc -mipmap";
    private const string cmd4 =
        "tc astc 6x6 E:\\202405\\ipc\\TextureCompressionV1\\bin\\input.png E:\\202405\\ipc\\TextureCompressionV1\\bin\\output4.astc  -mipmap";
	
    private const string errcmd = "tc bc 2 E:\\202405\\ipc\\TextureCompressionV1\\bin\\input.png E:\\202405\\ipc\\TextureCompressionV1\\bin\\output6.dds -mipmap";
#else
    #if UNITY_EDITOR_OSX || UNITY_STANDALONE_OSX
    private const string tcpath = "/Users/stone/work/c++/ipc/TextureCompressionV1/bin/YaTCompress";

    private const string cmd1 =
        "tc bc 1 /Users/stone/work/c++/ipc/TextureCompressionV1/bin/input.png /Users/stone/work/c++/ipc/TextureCompressionV1/bin/output1.dds";
    private const string cmd2 =
        "tc bc 3 /Users/stone/work/c++/ipc/TextureCompressionV1/bin/input.png /Users/stone/work/c++/ipc/TextureCompressionV1/bin/output2.dds";
    private const string cmd3 =
        "tc bc 7 /Users/stone/work/c++/ipc/TextureCompressionV1/bin/input.png /Users/stone/work/c++/ipc/TextureCompressionV1/bin/output3.dds";
    private const string cmd5 =
        "tc astc 4x4 /Users/stone/work/c++/ipc/TextureCompressionV1/bin/input.png /Users/stone/work/c++/ipc/TextureCompressionV1/bin/output5.astc";
    private const string cmd4 =
        "tc astc 6x6 /Users/stone/work/c++/ipc/TextureCompressionV1/bin/input.png /Users/stone/work/c++/ipc/TextureCompressionV1/bin/output4.astc";

    private const string errcmd =
        "tc bc 2 /Users/stone/work/c++/ipc/TextureCompressionV1/bin/input.png /Users/stone/work/c++/ipc/TextureCompressionV1/bin/output6.dds";
        
    
    #elif UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
    private const string tcpath = "C:/Users/wy301/source/repos/ipc/TextureCompressionV1/bin/YaTCompress.exe";

    private const string cmd1 =
        "tc bc 1 C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\input.png C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\output1.dds";
    private const string cmd2 =
        "tc bc 3 C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\input.png C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\output2.dds";
    private const string cmd3 =
        "tc bc 7 C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\input.png C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\output3.dds";
    private const string cmd5 =
        "tc astc 4x4 C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\input.png C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\output5.astc";
    private const string cmd4 =
        "tc astc 6x6 C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\input.png C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\output4.astc";

    private const string errcmd = "tc bc 2 C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\input.png C:\\Users\\wy301\\source\\repos\\ipc\\TextureCompressionV1\\bin\\output6.dds";
    #endif
#endif

    private void Start()
    {
        ExternalWorkerManager.SetTextureCompressorExecutablePath(tcpath);
        ExternalWorkerManager.GetInstance().Prepare();

        ExternalWorkerManager.Workload workload1 = new ExternalWorkerManager.Workload();
        workload1.WorkerType = 0;
        workload1.Cmd = cmd1;
        workload1.Callback = () => ProcessCompressedTexture(workload1.Cmd);

        ExternalWorkerManager.Workload workload2 = new ExternalWorkerManager.Workload();
        workload2.WorkerType = 0;
        workload2.Cmd = cmd2;
        workload2.Callback = () => ProcessCompressedTexture(workload2.Cmd);

        ExternalWorkerManager.Workload workload3 = new ExternalWorkerManager.Workload();
        workload3.WorkerType = 0;
        workload3.Cmd = cmd3;
        workload3.Callback = () => ProcessCompressedTexture(workload3.Cmd);

        ExternalWorkerManager.Workload workload4 = new ExternalWorkerManager.Workload();
        workload4.WorkerType = 0;
        workload4.Cmd = cmd4;
        workload4.Callback = () => ProcessCompressedTexture(workload4.Cmd);

        ExternalWorkerManager.Workload workload5 = new ExternalWorkerManager.Workload();
        workload5.WorkerType = 0;
        workload5.Cmd = cmd5;
        workload5.Callback = () => ProcessCompressedTexture(workload5.Cmd);

        ExternalWorkerManager.GetInstance().AddJob(workload1);
        ExternalWorkerManager.GetInstance().AddJob(workload2);
        ExternalWorkerManager.GetInstance().AddJob(workload3);
        ExternalWorkerManager.GetInstance().AddJob(workload4);
        ExternalWorkerManager.GetInstance().AddJob(workload5);
    }

    private void Update()
    {
        //error case
        if (Input.GetKeyDown(KeyCode.A))
        {
            ExternalWorkerManager.Workload workload = new ExternalWorkerManager.Workload();
            workload.WorkerType = 0;
            workload.Cmd = errcmd;
		    workload.Callback = () => ProcessCompressedTexture(workload.Cmd);
            ExternalWorkerManager.GetInstance().AddJob(workload);
        }
    }

    void ProcessCompressedTexture(string cmd)
    {
        UnityMainThreadDispatcher.Instance.Enqueue(() => {
            string[] argvs = cmd.Split(' ');
            if (argvs.Length >= 5)
            {
                string inputFilePath = argvs[3];
                string outputFilePath = argvs[4];

                bool dxtOrAstc = Path.GetExtension(outputFilePath).ToLower() == ".dds";

                var compressedBytes = File.ReadAllBytes(outputFilePath);
                Texture2D tex2D = dxtOrAstc ? DDSByteToTexture2D(compressedBytes) : ASTCByteToTexture2D(compressedBytes);
                tex2D.name = Path.GetFileNameWithoutExtension(outputFilePath);

#if UNITY_YAHAHA
                BuildToLDAndHDBundle(tex2D);
#else
                Debug.Log("Please use the Yahaha version Unity to package AS Texture");
#endif
            }
        });
    }

    Texture2D DDSByteToTexture2D(byte[] ddsBytes)
    {
        byte ddsSizeCheck = ddsBytes[4];
        if (ddsSizeCheck != 124)
            throw new System.Exception("Invalid DDS DXTn texture. Unable to read");

        int DDS_HEADER_SIZE = 128;
        int DDS_HEADER_DX10_SIZE = 20;

        int height = ddsBytes[12] | ddsBytes[13] << 8 | ddsBytes[14] << 16 | ddsBytes[15] << 24;
        int width = ddsBytes[16] | ddsBytes[17] << 8 | ddsBytes[18] << 16 | ddsBytes[19] << 24;
        int mipMapCount = ddsBytes[28] | ddsBytes[29] << 8 | ddsBytes[30] << 16 | ddsBytes[31] << 24;
        bool hasMipMaps = mipMapCount > 1;

        int pfFourCC = ddsBytes[84] | ddsBytes[85] << 8 | ddsBytes[86] << 16 | ddsBytes[87] << 24;
        bool hasDX10Header = (pfFourCC == ('D' | ('X' << 8) | ('1' << 16) | ('0' << 24)));

        TextureFormat format = TextureFormat.RGBA32;
        int headerSize = DDS_HEADER_SIZE;
        if (hasDX10Header)
        {
            headerSize += DDS_HEADER_DX10_SIZE;
            format = TextureFormat.BC7;
        }
        else
        {
            switch (pfFourCC)
            {
                case 0x31545844:
                    format = TextureFormat.DXT1;
                    break;
                case 0x35545844:
                    format = TextureFormat.DXT5;
                    break;
            }
        }

        byte[] dxtBytes = new byte[ddsBytes.Length - headerSize];
        System.Buffer.BlockCopy(ddsBytes, headerSize, dxtBytes, 0, ddsBytes.Length - headerSize);

        Texture2D texture = new Texture2D(width, height, format, hasMipMaps);
        texture.LoadRawTextureData(dxtBytes);
        texture.Apply();

        return texture;
    }

    private const int ASTC_HEADER_SIZE = 16;
    public Texture2D ASTCByteToTexture2D(byte[] astcBytes)
    {
        if (astcBytes[0] != 0x13 || astcBytes[1] != 0xAB || astcBytes[2] != 0xA1 || astcBytes[3] != 0x5C)
            throw new System.Exception("Invalid ASTC texture. Unable to read");

        int blockWidth = astcBytes[4];
        int blockHeight = astcBytes[5];

        int width = astcBytes[9] | (astcBytes[8] << 8) | (astcBytes[7] << 16);
        int height = astcBytes[12] | (astcBytes[11] << 8) | (astcBytes[10] << 16);

        TextureFormat format = GetASTCTextureFormat(blockWidth, blockHeight);

        int blockSize = blockWidth * blockHeight * 16 / 8; 
        int numBlocksWide = (width + blockWidth - 1) / blockWidth;
        int numBlocksHigh = (height + blockHeight - 1) / blockHeight;
        int minSize = numBlocksWide * numBlocksHigh * 16 + ASTC_HEADER_SIZE;

        bool hasMipMaps = astcBytes.Length > minSize;

        Texture2D texture = new Texture2D(width, height, format, hasMipMaps);

        byte[] astcData = new byte[astcBytes.Length - ASTC_HEADER_SIZE];
        System.Buffer.BlockCopy(astcBytes, ASTC_HEADER_SIZE, astcData, 0, astcBytes.Length - ASTC_HEADER_SIZE);

        texture.LoadRawTextureData(astcData);
        texture.Apply();

        return texture;
    }

    private TextureFormat GetASTCTextureFormat(int blockWidth, int blockHeight)
    {
        if (blockWidth == 4 && blockHeight == 4) return TextureFormat.ASTC_4x4;
        if (blockWidth == 5 && blockHeight == 5) return TextureFormat.ASTC_5x5;
        if (blockWidth == 6 && blockHeight == 6) return TextureFormat.ASTC_6x6;
        throw new System.Exception($"Unsupported ASTC block size: {blockWidth}x{blockHeight}");
    }

#if UNITY_YAHAHA
    public static int splitMipLevel = 6;
    void BuildToLDAndHDBundle(Texture2D texture2D)
    {
        if (texture2D.mipmapCount == 1)
        {
            Debug.Log("This texture has only one mipmap level; only one HD binary file will be generated for " + texture2D.name);

            byte[] highResBytes = texture2D.GetStreamedBinaryData(true, splitMipLevel);
            string folderPath = Path.Combine(Application.streamingAssetsPath, "AS_Texture");
            if (!Directory.Exists(folderPath))
            {
                Directory.CreateDirectory(folderPath);
            }
            string highResFilePath = Path.Combine(folderPath, texture2D.name + "_hd.bytes");

            File.WriteAllBytes(highResFilePath, highResBytes);
        }
        else if (texture2D.mipmapCount < splitMipLevel + 2)
        {
            Debug.Log("The specified splitMipLevel is not within the middle range of this texture's mipmap levels. Please reduce the splitMipLevel before splitting. Only one HD binary file will be generated for " + texture2D.name);

            byte[] highResBytes = texture2D.GetStreamedBinaryData(true, splitMipLevel);
            string folderPath = Path.Combine(Application.streamingAssetsPath, "AS_Texture");
            if (!Directory.Exists(folderPath))
            {
                Directory.CreateDirectory(folderPath);
            }
            string highResFilePath = Path.Combine(folderPath, texture2D.name + "_hd.bytes");

            File.WriteAllBytes(highResFilePath, highResBytes);
        }
        else
        {
            byte[] lowResBytes = texture2D.GetStreamedBinaryData(false, splitMipLevel);
            byte[] highResBytes = texture2D.GetStreamedBinaryData(true, splitMipLevel);

            string folderPath = Path.Combine(Application.streamingAssetsPath, "AS_Texture");
            if (!Directory.Exists(folderPath))
            {
                Directory.CreateDirectory(folderPath);
            }
            string lowResFilePath = Path.Combine(folderPath, texture2D.name + "_ld.bytes");
            string highResFilePath = Path.Combine(folderPath, texture2D.name + "_hd.bytes");

            File.WriteAllBytes(lowResFilePath, lowResBytes);
            File.WriteAllBytes(highResFilePath, highResBytes);
        }
    }
#endif
}
