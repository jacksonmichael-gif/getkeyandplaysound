# getkeyandplaysound
This repository is for ahegaokeyboard-a keyboard that printed sensual character each keys.
## Discription
(Note; This only works on Windows)  
This is for ahegaokeyboard and others.This contains files "ahegao.exe", "codetokey.json", "voicepath.json", "voices".if you want change the voice, you can add a wav_pcm files that's name writed in ANSI stringds in "voices", and rewrite "voicepath.json" your path(it is usually recommended to write it as a relative path from "ahgao.exe"). 
## How to use
1. Dounload "ahegao.zip" and unzip the file.
2. Do the "ahegao.exe" in the "ahegao".
3. Console will ask you which device you want to register.and if you press a key on the keyboard, thats keyboard will register.
4. If you press the key registered, a voice in "voicepath.json".
## for developper
It makes in visualstudio2022 comunity, and it is no portable because it uses winapi.  
A part of this code "loadWavtoBuffer" writed by AI, so I don't understud this code.If you understud this and think of something better way or how to port to other operating systems, please send a pull request.
## Acknowledgements
This project uses the nlohmann/json liblary by Niels Lohmann.  
Special thanks to the contributors of the project.  
Repository: [https://github.com/nlohmann/json.git](https://github.com/nlohmann/json.git)
