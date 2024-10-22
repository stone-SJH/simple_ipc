#include "Mutex.h"

Mutex::Mutex() {}

Mutex::~Mutex() {}

void Mutex::Lock() {
	m_Mutex.Lock();
}

void Mutex::Unlock() {
	m_Mutex.Unlock();
}

bool Mutex::TryLock() {
    bool locked = true;
    locked = m_Mutex.TryLock();
    return locked;
}

void Mutex::BlockUntilUnlocked() {
    Lock();
    Unlock();
}