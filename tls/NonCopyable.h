#pragma once

class NonCopyable {
public:
	NonCopyable() {}

private:
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};