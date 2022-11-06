#include "Exception.h"
#include "BackTrace.h"
#include "Core.h"
#include "Log.h"

#include <Windows.h>

namespace Utils
{
	static thread_local void* g_ExcludedExceptionType;

	static thread_local BackTrace tl_BackTrace;

	LONG VectoredHandler(EXCEPTION_POINTERS* ExceptionPointers);

	void HookThrow()
	{
		AddVectoredExceptionHandler(~0U, &VectoredHandler);

		try
		{
			throw Exception("Hehe", "ehhe");
		}
		catch ([[maybe_unused]] const Exception& e)
		{
		}
	}

	BackTrace& LastBackTrace() { return tl_BackTrace; }

	LONG VectoredHandler(EXCEPTION_POINTERS* ExceptionPointers)
	{
		if (ExceptionPointers->ExceptionRecord->ExceptionInformation[0] == 429065504ULL)
		{
			if (!g_ExcludedExceptionType)
				g_ExcludedExceptionType = reinterpret_cast<void*>(ExceptionPointers->ExceptionRecord->ExceptionInformation[2]);
			if (reinterpret_cast<void*>(ExceptionPointers->ExceptionRecord->ExceptionInformation[2]) != g_ExcludedExceptionType)
				tl_BackTrace = CaptureBackTrace(6, 32);
			else
				tl_BackTrace = {};
		}
		else
			tl_BackTrace = CaptureBackTrace(1, 32);
		return 0;
	}
} // namespace Utils
