# MySVE \- Podróż przez UE5 Rendering Pipeline

## Cel projektu

Zaimplementować własny **SceneViewExtension (SVE)** w Unreal Engine 5.7 zbudowanym ze źródeł, który:

1. Wpina się w pipeline renderowania przez oficjalny hook `PrePostProcessPass_RenderThread`  
2. Pobiera `SceneColor` jako UAV  
3. Przepuszcza go przez własny **compute shader**  
4. Podmienia każdy piksel na kolor czerwony

---

## Krok 1 \- Budowanie UE5 ze źródeł

### Co zrobiłem

Sklonowałem repozytorium UE 5.7 release z GitHub i zbudowałem silnik z kodu źródłowego w Visual Studio 2022 (Development Editor).

### Napotkane problemy

**Problem: Ogromny rozmiar downloadu (\~83GB)** Setup.bat pobiera pliki dla wszystkich platform z domyślnymi ustawieniami. **Rozwiązanie:** Użycie flag `--exclude` dla nieużywanych platform (Mac, Linux, Android, iOS itd.) oraz pliku `.gitdepsignore` do trwałego wykluczenia lokalizacji i nieużywanych binariów .NET. Rozmiar zredukowany do \~34GB. Dodatkowa optymalizacja: `git clone --depth=1` przy kolejnych klonach.

**Problem: Błędy kompilacji modułów UbaCoordinatorHorde i SlateViewer** Moduły związane z distributed build system (Horde) i standalone UI viewer failowały z kodem 6\. **Rozwiązanie:** Zignorowane \- oba moduły są opcjonalne i niepotrzebne do pracy z SVE. Edytor skompilował się poprawnie bez nich.

**Problem: Brak pamięci RAM podczas kompilacji (C3859, C1076)** Kompilator kończył się pamięcią przy równoległej kompilacji wielu modułów. **Rozwiązanie:** Zamknięcie innych aplikacji, ograniczenie równoległości w VS (`Maximum parallel project builds = 2`).

---

## Krok 2 \- Setup projektu i pluginu

### Co zrobiłem

Stworzyłem nowy projekt C++ `SVE_Sandbox` w UE5 oraz plugin `MySVE` przez `Edit → Plugins → Add` (Blank Plugin template).

### Napotkane problemy

**Problem: Błąd parsowania .uplugin przy Generate Project Files** `Failed to generate project files` z błędem JSON. **Rozwiązanie:** Znalazłem literówkę w pliku `.uplugin` i poprawiłem JSON.

**Problem: IntelliSense pokazywał czerwone błędy na wszystkich nagłówkach UE** E1696: `nie można otworzyć pliku źródło "SceneViewExtension.h"`. **Rozwiązanie:** Usunąłem foldery `.vs/`, `Binaries/`, `Intermediate/` z projektu i ponownie uruchomiłem Generate Project Files \- IntelliSense przebudował cache z pełną wiedzą o ścieżkach UE.

**Problem: VSIT (Visual Studio Integration Tool) nie mógł dodać `.editorconfig`** Narzędzie failowało przy próbie automatycznego dodania pliku. **Rozwiązanie:** Ręcznie stworzyłem plik `.editorconfig` w folderze projektu z gotową zawartością konwencji nazewnictwa UE (`F` prefix dla struktur, `U` dla UObject, `A` dla Aktorów itd.).

---

## Krok 3 \- Pisanie kodu SVE i Compute Shadera

### Co zrobiłem

Napisałem trzy pliki:

- `MySVEExtension.h/.cpp` \- klasa SVE dziedzicząca z `FSceneViewExtensionBase`  
- `MySVEModule.cpp` \- moduł rejestrujący SVE  
- `RedOverlay.usf` \- compute shader w HLSL malujący każdy piksel na czerwono

Dodałem CVAR `MySVE.Enable` do włączania/wyłączania efektu z konsoli UE w runtime.

### Napotkane problemy

**Problem: `DECLARE_GLOBAL_SHADER` nie działa w pluginach** Błąd kompilacji \- makro działa tylko w kodzie silnika. **Rozwiązanie:** Zastąpiłem przez `DECLARE_SHADER_TYPE` i `IMPLEMENT_SHADER_TYPE` \- oficjalny sposób dla zewnętrznych pluginów opisany w dokumentacji RDG 101\.

**Problem: `#include "PostProcess/PostProcessing.h"` \- No such file** Header prywatny w UE5, niedostępny publicznie. **Rozwiązanie:** Podmieniłem na `#include "PostProcess/PostProcessMaterialInputs.h"` \- znalazłem go przez podglądanie includeów w pluginach Epic'a (ColorCorrectRegions, CompositeCore).

**Problem: `#include "SceneTextures.h"` \- No such file** `SceneTextures.h` przeniesiony do `Renderer\Internal\` w UE5.3+, niedostępny publicznie. **Rozwiązanie:** Zmieniłem podejście z `PreRenderViewFamily_RenderThread` na `PrePostProcessPass_RenderThread` \- oficjalny publiczny hook SVE który dostarcza SceneColor bezpośrednio przez `FPostProcessingInputs` bez potrzeby sięgania do prywatnych headerów.

**Problem: `FPostProcessingInputs` niezdefiniowany** Typ zdefiniowany w `PostProcessInputs.h` który siedzi w `Renderer\Internal\PostProcess\` \- znowu prywatny. **Rozwiązanie:** Dodałem do `Build.cs` ścieżki do prywatnych headerów Renderera:

PrivateIncludePathModuleNames.Add("Renderer");

PrivateIncludePaths.Add(Path.Combine(EnginePath, "Source/Runtime/Renderer/Internal"));

PrivateIncludePaths.Add(Path.Combine(EnginePath, "Source/Runtime/Renderer/Internal/PostProcess"));

Technicznie jest to "hack" \- Epic celowo schował te headery, ale nie zablokował dostępu przez ścieżki.

**Problem: Podwójnie zdefiniowane konstruktory (C2535)** `SHADER_USE_PARAMETER_STRUCT` generuje konstruktory automatycznie, ręczne dodanie powoduje konflikt. **Rozwiązanie:** Usunąłem ręcznie napisane konstruktory z klasy shadera.

---

## Krok 4 \- Uruchamianie edytora i debugowanie

### Napotkane problemy i jak je rozwiązałem

**Problem: Plugin 'MySVE' failed to load \- DLL nie mogła być załadowana** Edytor crashował przy ładowaniu pluginu. **Rozwiązanie:** Zmieniłem `LoadingPhase` w `.uplugin` z `PostEngineInit` na `PostConfigInit` \- shadery muszą być rejestrowane przez `IMPLEMENT_SHADER_TYPE` zanim engine zainicjalizuje system shaderów. Z `PostEngineInit` było za późno.

**Problem: Fatal error \- `Couldn't find source file of virtual shader path '/Plugin/MySVE/RedOverlay.usf'`** UE nie wiedział gdzie fizycznie szukać pliku `.usf` pomimo `ShaderDirectories` w `.uplugin`. **Rozwiązanie:** Ręcznie zarejestrowałem ścieżkę shadera w `StartupModule()`:

FString PluginShaderDir \= FPaths::Combine(

    IPluginManager::Get().FindPlugin(TEXT("MySVE"))-\>GetBaseDir(),

    TEXT("Shaders")

);

AddShaderSourceDirectoryMapping(TEXT("/Plugin/MySVE"), PluginShaderDir);

`ShaderDirectories` w `.uplugin` nie wystarcza w UE5 \- trzeba jawnie wywołać `AddShaderSourceDirectoryMapping`.

**Problem: Edytor otwierał się poprawnie ale viewport był normalny \- brak czerwonego ekranu** Plugin ładował się, shadery kompilowały się, a efekt nie działał. Viewport wyglądał zupełnie normalnie jakby SVE w ogóle nie istniało. W Output Logu był widoczny błąd na czerwono który wskazywał na przyczynę:

LogOutputDevice: Error: Ensure condition failed: GEngine

\[File: SceneViewExtension.cpp\] \[Line: 70\]

UnrealEditor-MySVE.dll\!FMySVEModule::StartupModule()

\[MySVEModule.cpp:20\]

Ensure `GEngine` w `SceneViewExtension.cpp` linia 70 \- UE próbował zarejestrować extension ale `GEngine` jeszcze nie istniał w tym momencie inicjalizacji. Błąd był czytelny w logu, ale łatwo go przeoczyć bo edytor mimo wszystko ładował się dalej bez crashu.

Przyczyna była subtelna: `FSceneViewExtensions::NewExtension<FMySVEExtension>()` było wywoływane w `StartupModule()` \- czyli w momencie gdy UE ładuje moduł pluginu. Problem w tym że w tej fazie `GEngine` i renderer nie są jeszcze w pełni zainicjalizowane. `NewExtension` wołało ensure na `GEngine`, extension "rejestrowało się" ale renderer UE jeszcze nie był gotowy do przyjmowania nowych SVE \- i po prostu je ignorował. Żadnego błędu, żadnego warning, cisza.

**Rozwiązanie:** Opóźniłem tworzenie extension do momentu gdy engine jest w pełni gotowy, przez podpięcie się pod delegat `FCoreDelegates::OnPostEngineInit`:

virtual void StartupModule() override

{

    // Najpierw rejestracja shaderow \- to musi byc w StartupModule

    FString PluginShaderDir \= FPaths::Combine(

        IPluginManager::Get().FindPlugin(TEXT("MySVE"))-\>GetBaseDir(),

        TEXT("Shaders")

    );

    AddShaderSourceDirectoryMapping(TEXT("/Plugin/MySVE"), PluginShaderDir);

    // SVE tworzymy DOPIERO gdy engine jest w pelni zainicjalizowany

    FCoreDelegates::OnPostEngineInit.AddLambda(\[this\]()

    {

        MySVEExtension \= FSceneViewExtensions::NewExtension\<FMySVEExtension\>();

    });

}

Kluczowy wniosek: rejestracja shaderów (`AddShaderSourceDirectoryMapping`) musi być w `StartupModule` \- ale tworzenie SVE musi czekać na `OnPostEngineInit`. Dwie różne rzeczy, dwa różne momenty inicjalizacji.

---

## Rezultat końcowy

Zbudowałem działający plugin SVE który:

- Rejestruje się w pipeline renderowania UE5 przez oficjalny `PrePostProcessPass_RenderThread`  
- Pobiera `SceneColor` jako `FRDGTextureRef` z `FPostProcessingInputs`  
- Tworzy UAV (`FRDGTextureUAVRef`) z SceneColor  
- Odpala compute shader z grupami 8×8 wątków pokrywającymi cały ekran (DivUp)  
- Każdy wątek zapisuje `float4(1,0,0,1)` do swojego piksela  
- Efekt można włączać/wyłączać w konsoli UE: `MySVE.Enable 0/1`

---

## Kluczowe wnioski z całej podróży

**"Czy to normalne?" \- TAK, absolutnie.**

Praca z UE5 rendering pipeline z source buildem to seria problemów które są standardem w tej branży:

**Epic chowa coraz więcej headerów w `Internal/`** \- każda nowa wersja UE5 przenosi kolejne typy do prywatnych folderów. Trzeba szukać publicznych alternatyw albo sięgać po PrivateIncludePaths. Najlepszą metodą jest podglądanie jak Epic sam robi to w swoich pluginach (ColorCorrectRegions, CompositeCore).

**Dokumentacja jest przestarzała** \- większość materiałów online o SVE i RDG jest pisana pod UE4 lub wczesny UE5. `SceneRenderTargets.h`, `DECLARE_GLOBAL_SHADER`, stare podejście przez `PreRenderViewFamily` \- to wszystko jest już outdated.

**LoadingPhase ma ogromne znaczenie** \- rejestracja shaderów przez `IMPLEMENT_SHADER_TYPE` musi się odbyć w `PostConfigInit`, a tworzenie SVE musi czekać na `OnPostEngineInit`. Kolejność inicjalizacji w UE jest krytyczna.

**IntelliSense vs kompilator** \- IntelliSense często myli się przy kodzie UE (czerwone podkreślenia mimo poprawnego kodu). Zawsze warto próbować skompilować zanim zacznie się debugować błędy IntelliSense.

**RDG 101 to must-read** \- prezentacja Epic'a o Render Dependency Graph (dostępna na mcro.de/c/rdg) to najlepsza dokumentacja jak działa nowoczesny pipeline renderowania w UE5. Mimo że niektóre przykłady są z UE4, koncepcje są aktualne.  
