#include <windows.h>
#include <direct.h>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <dsound.h>
#include <algorithm>
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")

std::ifstream codetokey("./codetokey.json");
std::ifstream voicepath("./voicepath.json");
nlohmann::json codetokey_json;
nlohmann::json voicepath_json;
std::unordered_map<std::string, std::string> keyToVoice;
LPDIRECTSOUND8 g_pDS = nullptr;
std::unordered_map<std::string, LPDIRECTSOUNDBUFFER> keyToBuffer;

const UINT keyLayout[][14] = {
	{VK_ESCAPE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, VK_BACK},
	{VK_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', VK_OEM_4, VK_OEM_6, VK_RETURN},
	{VK_CAPITAL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', VK_OEM_1, VK_OEM_7, VK_OEM_3, 0},
	{VK_LSHIFT, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2, VK_RSHIFT, 0, 0},
	{VK_LCONTROL, VK_LWIN, VK_LMENU, VK_SPACE, VK_RMENU, VK_RWIN, VK_APPS, VK_RCONTROL, 0, 0, 0, 0, 0, 0}
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_CLOSE) {
		DestroyWindow(hwnd);
	}
	else if (msg == WM_DESTROY) {
		PostQuitMessage(0);
	}
	else if (msg == WM_COMMAND) {
		int id = LOWORD(wParam);
		int row = id / 300;
		int col = id % 300;

		if (row >= 0 && row < 5 && col >= 0 && col < 14) {
			if (id == 0) return 0; // 無効キー

			std::string keycode = codetokey_json[std::to_string(id)];
			// 以下、voicepath_json[keycode] を使って処理

			if (voicepath_json.contains(keycode)) {
				std::string path = voicepath_json[keycode];
				PlaySoundA(path.c_str(), NULL, SND_FILENAME | SND_ASYNC);
				OPENFILENAME ofn;       // 構造体
				wchar_t szFile[260] = { 0 }; // ファイルパス格納用

				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = NULL;
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
				ofn.lpstrFilter = L"すべてのファイル\0*.*\0テキストファイル\0*.TXT\0";
				ofn.nFilterIndex = 1;
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

				if (GetOpenFileName(&ofn)) {
					std::wcout << L"選択されたファイル: " << ofn.lpstrFile << std::endl;

					// ファイルを読み込む（ストリームとして）
					std::wifstream file(ofn.lpstrFile);
					if (file) {
						char charpath[100];
						std::setlocale(LC_ALL, ""); // ロケールを設定
						size_t convertedChars;
						wcstombs_s(&convertedChars, charpath, sizeof(charpath), ofn.lpstrFile, _TRUNCATE);
						std::cout << "選択されたファイルパス: " << charpath << std::endl;
						voicepath_json[path] = (std::string)charpath;
						file.close();
					}
					else {
						std::wcerr << L"ファイルを開けませんでした。" << std::endl;
					}
				}
				else {
					std::wcerr << L"ファイルが選択されませんでした。" << std::endl;
				}

			}
		}
	}
	else {
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
wchar_t GetCharFromVK(UINT vk, HKL layout) {
	BYTE keyboardState[256] = {};
	wchar_t buffer[4] = {};
	int result = ToUnicodeEx(vk, MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, layout),
		keyboardState, buffer, 4, 0, layout);
	return result > 0 ? buffer[0] : L'?';
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
	HKL layout = GetKeyboardLayout(0); // 現在のスレッドのキーボードレイアウト
	const wchar_t CLASS_NAME[] = L"Sample Window Class";
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);
	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"Learn to Program Windows",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);
	if (hwnd == NULL) {
		std::cerr << "ウィンドウの作成に失敗しました。" << std::endl;
		return 0;
	}
	InitDirectSound(hwnd);
	if (!InitDirectSound(hwnd)) {
		std::cerr << "DirectSoundの初期化に失敗しました" << std::endl;
		return -1;
	}
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
	/*HWND hwndButton = CreateWindowEx(
		0,
		L"BUTTON",             // ボタンのクラス名
		L"クリックしてね",     // ボタンに表示するテキスト
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		50, 50, 150, 30,       // x, y, 幅, 高さ
		hwnd,                 // 親ウィンドウ
		(HMENU)ID_BUTTON1,    // ボタンの識別子
		wc.hInstance,
		NULL
	);*/
	int x = 50, y = 50;
	const int keyWidth = 40, keyHeight = 40, spacing = 5;

	for (int row = 0; row < 5; ++row) {
		x = 50;
		for (int col = 0; col < 14; ++col) {
			UINT vk = keyLayout[row][col];
			if (vk == 0) {
				x += keyWidth + spacing;
				continue;
			}

			wchar_t label[2] = { GetCharFromVK(vk, layout), 0 };

			HWND hButton = CreateWindowEx(
				0, L"BUTTON", label,
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				x, y, keyWidth, keyHeight,
				hwnd, (HMENU)(row + col * 300), wc.hInstance, NULL//keylayoutの最大値以上の数を用いてrow, colと単射な変換を行う
			);

			x += keyWidth + spacing;
		}
		y += keyHeight + spacing;
	}
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}