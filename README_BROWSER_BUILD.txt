FRUTIGER AERO WORLD - BROWSER BUILD

This package is set up so you can compile the PSP game without installing
PSP development tools on your Chromebook.

Browser workflow:
1. Create a GitHub repository in your browser.
2. Upload this project's files.
3. Make sure the file
   .github/workflows/build.yml
   is present exactly at that path.
4. Open the repository's Actions tab.
5. Choose "Build Frutiger Aero World for PSP".
6. Click "Run workflow".
7. When the workflow finishes, download the artifact named:
   FrutigerAeroWorld-PSP
8. Inside the downloaded artifact is EBOOT.PBP.

Put the resulting EBOOT.PBP on the PSP at:
PSP/GAME/FrutigerAeroWorld/EBOOT.PBP

Then launch:
Game -> Memory Stick -> Frutiger Aero World

The GitHub workflow uses the PSPDEV project's published PSP development
container to build the project, so the Chromebook only needs a web browser.
