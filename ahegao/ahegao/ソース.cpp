#include <windows.h>
#include <direct.h>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <dsound.h>
#include <algorithm>
HANDLE devicehandle;
std::ifstream codetokey("./codetokey.json");
std::ifstream voicepath("./voicepath.json");
nlohmann::json codetokey_json;
nlohmann::json voicepath_json;
std::unordered_map<std::string, std::string> keyToVoice;
LPDIRECTSOUND8 g_pDS = nullptr;
std::unordered_map<std::string, LPDIRECTSOUNDBUFFER> keyToBuffer;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_INPUT) {
        UINT dwSize = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        BYTE* lpb = new BYTE[dwSize];
        if (lpb == nullptr)  return 0;

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
            RAWINPUT* raw = (RAWINPUT*)lpb;
            static bool done = false;
            if (!done) {
                if (raw->header.dwType == RIM_TYPEKEYBOARD) {
                    devicehandle = raw->header.hDevice;
					printf("登録しました\ndevicehandle: %p\n", devicehandle);//一行にprintf()は一つ!<-as先生
                    done = true;
                }
            }
            if (raw->header.dwType == RIM_TYPEKEYBOARD &&devicehandle == raw->header.hDevice) {
				//std::cout << raw->data.keyboard.VKey << std::endl;
                std::string keycode = std::to_string(raw->data.keyboard.VKey);
				//std::string str = voicepath_json.value((std::string)codetokey_json.value(keycode, "none"), "none");
                    auto it = keyToBuffer.find(keycode);
                    if (it != keyToBuffer.end()) {
                        if (!raw->data.keyboard.Flags) {
                            std::cout << "device: " << raw->header.hDevice << std::endl;
                            std::cout << "KeyCode: " << keycode << std::endl;
                            //it->second->SetVolume(DSBVOLUME_MAX); // 最大音量(いたずら用)
                            it->second->SetCurrentPosition(0); // 先頭から再生
                            auto ds = it->second->Play(0, 0, 0);
                            DWORD status;
                            it->second->GetStatus(&status);


                            if (FAILED(ds)) {
                                std::cerr << "サウンドの再生に失敗しました: HRESULT=" << std::hex << ds << std::endl;
                            }
                            std::cout << "PlayingSound: " << voicepath_json[(std::string)codetokey_json[keycode]] << std::endl;//<=これは動作が遅くテスト用途でのみ有効化
                        }
                    }
                    else {
						std::cout << "登録されていないキーです、有効にしたい場合はjsonファイルに書き加えてください" << std::endl;
                    }
            }
        }
        delete[] lpb;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
bool InitDirectSound(HWND hwnd) {
    if (FAILED(DirectSoundCreate8(NULL, &g_pDS, NULL))) return false;
    if (FAILED(g_pDS->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))) return false;
    return true;
}
LPDIRECTSOUNDBUFFER8 LoadWAVToBuffer(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "ファイルが開けません: " << filename << std::endl;
        return nullptr;
    }

    char chunkId[5] = {};
    DWORD chunkSize;
    char format[5] = {};
    file.read(chunkId, 4); file.read((char*)&chunkSize, 4); file.read(format, 4);
    chunkId[4] = '\0'; format[4] = '\0';

    if (strcmp(chunkId, "RIFF") != 0 || strcmp(format, "WAVE") != 0) {
        std::cerr << "RIFF/WAVE ヘッダが不正です" << std::endl;
        return nullptr;
    }

    WAVEFORMATEX wf = {};
    BYTE* audioData = nullptr;
    DWORD dataSize = 0;

    while (file.read(chunkId, 4)) {
        chunkId[4] = '\0';
        file.read((char*)&chunkSize, 4);
        std::streampos chunkStart = file.tellg();

        if (strcmp(chunkId, "fmt ") == 0) {
            DWORD fmtReadSize = std::min<DWORD>(chunkSize, sizeof(WAVEFORMATEX));
            file.read((char*)&wf, fmtReadSize);
            file.seekg(chunkStart + static_cast<std::streamoff>(chunkSize), std::ios::beg);
        }
        else if (strcmp(chunkId, "data") == 0) {
            audioData = new BYTE[chunkSize];
            file.read((char*)audioData, chunkSize);
            dataSize = chunkSize;
        }
        else {
            file.seekg(chunkStart + static_cast<std::streamoff>((chunkSize + 1) & ~1), std::ios::beg);
        }
    }

    if (!audioData || dataSize == 0) {
        std::cerr << "data チャンクが見つからないか、サイズが 0 です" << std::endl;
        return nullptr;
    }

    DSBUFFERDESC dsbd = {};
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
    dsbd.dwBufferBytes = dataSize;
    dsbd.lpwfxFormat = &wf;

    LPDIRECTSOUNDBUFFER pBuffer = nullptr;
    if (FAILED(g_pDS->CreateSoundBuffer(&dsbd, &pBuffer, NULL))) {
        std::cerr << "CreateSoundBuffer に失敗しました" << std::endl;
        delete[] audioData;
        return nullptr;
    }

    void* ptr1; DWORD size1;
    if (SUCCEEDED(pBuffer->Lock(0, dataSize, &ptr1, &size1, NULL, NULL, 0))) {
        memcpy(ptr1, audioData, size1);
        pBuffer->Unlock(ptr1, size1, NULL, 0);
    }
    else {
        std::cerr << "バッファのロックに失敗しました" << std::endl;
        pBuffer->Release();
        delete[] audioData;
        return nullptr;
    }

    delete[] audioData;

    // IDirectSoundBuffer8 にアップグレード
    LPDIRECTSOUNDBUFFER8 pBuffer8 = nullptr;
    if (SUCCEEDED(pBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&pBuffer8))) {
        pBuffer->Release();
        return pBuffer8;
    }
    else {
        std::cerr << "IDirectSoundBuffer8 へのキャストに失敗しました" << std::endl;
        pBuffer->Release();
        return nullptr;
    }
}

int main() {
	std::cout << "初期設定中" << std::endl;
    // ウィンドウクラス登録
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"RawInputClass";
    RegisterClass(&wc);

    // 非表示ウィンドウ作成
    HWND hwnd = CreateWindowEx(0, L"RawInputClass", L"RawInputWindow", 0,
        0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, NULL);
    
	// DirectSound初期化
	InitDirectSound(hwnd);
    if (!InitDirectSound(hwnd)) {
        std::cerr << "DirectSoundの初期化に失敗しました" << std::endl;
        return -1;
    }
    // RawInputデバイス登録
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01; // Generic desktop controls
    rid.usUsage = 0x06;     // Keyboard
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
    if (!codetokey.is_open())
        throw new std::exception("Failed open file.");

    if (!nlohmann::json::accept(codetokey))
        throw new std::exception("jsonのフォーマットが不正");
    codetokey.seekg(0, std::ios::beg);
	codetokey_json = nlohmann::json::parse(codetokey);
    if (!voicepath.is_open())
        throw new std::exception("Failed open file.");

    if (!nlohmann::json::accept(voicepath))
        throw new std::exception("jsonのフォーマットが不正");
    voicepath.seekg(0, std::ios::beg);
	voicepath_json = nlohmann::json::parse(voicepath);
    for (const auto& item : codetokey_json.items()) {
        std::string keycode = item.key();
        std::string keyname = item.value();

        if (!voicepath_json.contains(keyname)) {
            std::cerr << "voicepath_json に " << keyname << " が存在しません" << std::endl;
            continue;
        }

        std::string path = voicepath_json[keyname];
        std::cout << "登録: " << keycode << " → " << path << std::endl;

        LPDIRECTSOUNDBUFFER buffer = LoadWAVToBuffer(path);
        if (buffer) {
            keyToBuffer[keycode] = buffer;
            std::cout << "バッファ登録成功: " << keycode << std::endl;
        }
        else {
            std::cerr << "バッファ作成失敗: " << path << std::endl;
        }
    }
    std::cout << "入力するデバイスを登録してください" << std::endl;


    // メッセージループ
    while (true) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // 他の処理があればここに入れる
    }

    return 0;
}