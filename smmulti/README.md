# Sound Motel Multidrive

Distorsión multibanda original (VST3 en Windows y Mac, AU en Mac).

- 3 bandas (crossover Linkwitz-Riley de 4º orden, dos puntos de cruce ajustables).
- Por banda: modelo de distorsión (Soft Clip, Hard Clip, Tube, Fold, Rectify, Bit Crush, Sine, Fuzz), DRIVE y LEVEL.
- Generador de feedback (peine resonante con saturación): FEEDBACK y FB FREQ.
- MIX y OUTPUT globales.

## Obtener los instaladores
1. Crea un repositorio NUEVO en GitHub (por ejemplo "sound-motel-multidrive") y sube el contenido de esta carpeta.
2. Si no se subió la carpeta oculta `.github`, crea `.github/workflows/build.yml` pegando el contenido de `COPIA_VISIBLE_build.yml`.
3. Actions -> Build installers -> Run workflow.
4. Descarga `SoundMotelMultidrive-1.0.0-Windows-Setup.exe` y `SoundMotelMultidrive-1.0.0-macOS.pkg` desde "Artifacts".

## Prueba del motor de audio (sin JUCE)
    g++ -O2 -std=c++17 test/test_engine.cpp -o test_engine && ./test_engine
