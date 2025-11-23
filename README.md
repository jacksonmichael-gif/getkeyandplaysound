# getkeyandplaysound
This repository was developed for make your keyboard sensual and enjoyable.
## Discription
(Note; This only works on Windows)  
This release contains the executable file "ahegao.exe", "makeprofile.html", and a folder called "profiles".  
If you want to add a new voice, create a profile using "makeprofile.html" or make the changes directly in the profile.
## How to use
1. Dounload "release.zip" and unzip the file.
2. Do the "ahegao.exe" in the "release".
3. Console will ask you which profiles you want to use,and let the profilename in profiles at console to use the profile.  
If you want to add a new profile, you can make a new profile as zipped file with using "makeprofile.html".
Please open "makeprofile.html" on chromium browser and double click key and select a wav format file, this file will  be registered in this key.
Give the profile a name and save it as a .zip file.
Unzip the saved zip file to "profiles".
## for developper
It makes in visualstudio2022 comunity, and it is no portable because it uses winapi.  
A part of this code "loadWavtoBuffer" writed by AI, so I don't understud this code.If you understud this and think of something better way or how to port to other operating systems, please send a pull request.  
This solution"s debagging working dhirectory is in ahegao.exe, please move the containts in "toplayvoice" to "ahegao/x64/debug" when debagging.  
"makeprofile.html" wrote in HTML and JavaScript.  
considering implementing a profile transfer feature.
## Acknowledgements
This project uses the nlohmann/json liblary by Niels Lohmann.  
Special thanks to the contributors of the project.  
Repository: [https://github.com/nlohmann/json.git](https://github.com/nlohmann/json.git)
