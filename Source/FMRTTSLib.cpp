#define _ATL_APARTMENT_THREADED

#include "FMRTTSLib.h"

#include "Windows/AllowWindowsPlatformTypes.h"

// see "SAPI (or other users of atlmfc) in UE4 with VS2017/2019"
// https://inu-games.com/2018/10/15/sapi-in-ue4/
#pragma warning(push)
#pragma warning(disable: 4191)
#pragma warning(disable: 4996)
#ifndef DeleteFile
#define DeleteFile DeleteFileW
#endif
#ifndef MoveFile
#define MoveFile MoveFileW
#endif
#ifndef LoadString
#define LoadString LoadStringW
#endif
#ifndef InterlockedDecrement
#define InterlockedDecrement _InterlockedDecrement
#endif
#ifndef InterlockedIncrement
#define InterlockedIncrement _InterlockedIncrement
#endif
#ifndef GetMessage
#define GetMessage GetMessageW
#endif

#include <windows.h>
#include <sapi.h>
#include <sphelper.h>

#undef DeleteFile
#undef MoveFile
#undef LoadString
#undef InterlockedDecrement
#undef InterlockedIncrement
#undef GetMessage
#pragma warning(pop)

namespace FMRTTSLib
{
	class TTSException {};
	
	char* FMRTTSLibMain::TextToWav(const wchar_t* voiceRequiredAttributes, const wchar_t* voiceOptionalAttributes, long rate, const wchar_t* textToRender, ULONG* pBytesRead)
	{
		if (FAILED(::CoInitialize(NULL))) return NULL;
		
		try
		{
			char* r = TextToWavInner(voiceRequiredAttributes, voiceOptionalAttributes, rate, textToRender, pBytesRead);
			::CoUninitialize();
			return r;
		}
		catch (TTSException)
		{
			::CoUninitialize();
			return NULL;
		}
	}

	char* FMRTTSLibMain::TextToWavInner(const wchar_t* voiceRequiredAttributes, const wchar_t* voiceOptionalAttributes, long rate, const wchar_t* textToRender, ULONG* pBytesRead)
	{
		HRESULT hr;
		CComPtr<ISpVoice> cpVoice; //Will send data to ISpStream
		CComPtr<ISpStream> cpStream; //Will contain IStream
		CComPtr<IStream> cpBaseStream; //raw data
		ISpObjectToken* cpToken(NULL); //Will set voice characteristics

		GUID guidFormat;
		WAVEFORMATEX* pWavFormatEx = nullptr;

		hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
		checkAndThowException(hr);

		hr = SpFindBestToken(SPCAT_VOICES, voiceRequiredAttributes, voiceOptionalAttributes, &cpToken);
		if (hr == SPERR_NOT_FOUND)
		{
			// SPCAT_VOICES is defined in sapi.h as L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Voices"
			// Not all English Windows installations include voices in that registry location. (e.g. English(Canada) doesn't)
			// The more modern(?) narration system uses the Speech_OneCore "mobile" voices
			// This is where voices installed via Windows Settings->Time&Language->Speech are placed
			// We will look there too
			const wchar_t* ONECORE_VOICES = L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices";

			hr = SpFindBestToken(ONECORE_VOICES, voiceRequiredAttributes, voiceOptionalAttributes, &cpToken);
		}
		if (FAILED(hr)) return NULL;

		hr = cpVoice->SetVoice(cpToken);
		cpToken->Release();
		checkAndThowException(hr);

		cpVoice->SetRate(rate);

		hr = cpStream.CoCreateInstance(CLSID_SpStream);
		checkAndThowException(hr);

		hr = CreateStreamOnHGlobal(NULL, TRUE, &cpBaseStream);
		checkAndThowException(hr);

		hr = SpConvertStreamFormatEnum(SPSF_44kHz16BitMono, &guidFormat, &pWavFormatEx);
		checkAndThowException(hr);

		hr = cpStream->SetBaseStream(cpBaseStream, guidFormat, pWavFormatEx);
		checkAndThowException(hr);

		hr = cpVoice->SetOutput(cpStream, FALSE);
		checkAndThowException(hr);

		// While the default XML auto-detection works fine, it throws a non-fatal and rather non-descriptive exception:
		// From KernelBase.dll: 0xE0000002 (parameters: 0xFFFFFFFF80045003)
		// Explicitly specifying the parsing strategy quells it.
		hr = cpVoice->Speak(textToRender, SPF_ASYNC | SPF_IS_XML | SPF_PARSE_SAPI, NULL);
		checkAndThowException(hr);
		hr = cpVoice->WaitUntilDone(1000);
		checkAndThowException(hr);

		// Uncomment below to directly output speech
		//cpVoice->SetOutput(NULL, FALSE);
		//cpVoice->SpeakStream(cpStream, SPF_DEFAULT, NULL);

		LARGE_INTEGER a = { 0 };
		hr = cpStream->Seek(a, STREAM_SEEK_SET, NULL);
		checkAndThowException(hr);

		STATSTG stats;
		cpStream->Stat(&stats, STATFLAG_NONAME);
		
		ULONG sSize = stats.cbSize.LowPart;
		
		char *pBuffer = new char[sSize];
		cpStream->Read(pBuffer, sSize, pBytesRead);

		return pBuffer;
	}

	void FMRTTSLibMain::checkAndThowException(HRESULT hr)
	{
		if (FAILED(hr)) throw TTSException();
	}
}

#include "Windows/HideWindowsPlatformTypes.h"
