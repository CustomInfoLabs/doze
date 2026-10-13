# Doze
**Il tuo PC. I tuoi tempi. / Your PC. Your time.**

[Italiano](#italiano) · [English](#english)

[Sito ufficiale / Official website](https://custominfo.it/)

## Italiano

Doze è una utility Windows di **CustomInfo** per programmare spegnimento, riavvio, sospensione e ibernazione attraverso un countdown annullabile.

**1.0.0 — Prima release.**

### Funzionalità

- Quattro azioni: spegnimento, riavvio, sospensione e ibernazione.
- Durata impostabile da tastiera, pulsanti e intervalli rapidi.
- Annullamento del countdown.
- Blocco del PC e annullamento degli spegnimenti pianificati da Windows.
- Controllo dello stato dell’ibernazione, con attivazione e disattivazione su richiesta.
- Interfaccia italiana e inglese, temi chiaro e scuro.
- Avvio rapido dal menu contestuale, se selezionato durante l’installazione.

Le operazioni energetiche dipendono dalle capacità del PC e dalle impostazioni di Windows. Salva il lavoro prima di programmare spegnimento o riavvio.

### Sorgente e trasparenza

Questo repository contiene il sorgente completo di Doze, inclusa l’interfaccia grafica.

Il codice è pubblicato per consentire di esaminare il funzionamento del programma, verificare le operazioni richieste a Windows e ricompilare localmente il progetto.

La pubblicazione del sorgente non costituisce, da sola, una certificazione di sicurezza o della corrispondenza di un eseguibile al codice. Ogni pacchetto ufficiale dovrà essere associato alla versione del sorgente da cui è stato prodotto.

### Struttura

```text
src/          Applicazione nativa e gestione delle azioni
assets/       Icone, font e risorse incorporate
packaging/    Script di compilazione, installer e guida utente
tests/        Test del motore
project.json  Informazioni e versione del prodotto
CHANGELOG.md  Note delle versioni
LICENSE.txt   Condizioni d’uso e consultazione del sorgente
```

### Compilazione

Doze utilizza **C++20, Win32 e GDI+**. La compilazione richiede Windows e la toolchain prevista dal progetto:

- Zig 0.15.2.
- nlohmann JSON 3.12.0.
- Inno Setup 6.7.3 per creare l’installer, nel rispetto delle sue condizioni d’uso.

Dalla radice del repository, in PowerShell:

```powershell
.\packaging\prepare-toolchain.ps1 -Destination ..\doze-toolchain
```

Per compilare l’applicazione ed eseguire i test:

```powershell
.\packaging\build.ps1 `
  -Toolchain ..\doze-toolchain `
  -Output ..\doze-build\Doze `
  -Work ..\doze-build\work `
  -Tests
```

Per creare anche il setup, aggiungi `-Installer` allo stesso comando.

I test automatizzati del motore e dell’interfaccia non eseguono spegnimenti, riavvii, sospensioni o ibernazioni reali. Le prove sul sistema e dell’installer sono verifiche separate.

### Installazione e utilizzo

L’installer ufficiale installa Doze per l’utente Windows corrente. L’aggiunta al menu **Shift + tasto destro** è opzionale.

La guida bilingue `README.txt` inclusa nella cartella installata descrive avvio, utilizzo, preferenze e disinstallazione.

### Licenza

Doze è gratuito per uso personale e professionale.

Il sorgente può essere consultato, analizzato e compilato localmente per verifica. Il riutilizzo del codice in altri prodotti, la distribuzione di versioni modificate e il riutilizzo degli asset grafici originali richiedono l’autorizzazione prevista dalla licenza.

Consulta [LICENSE.txt](LICENSE.txt) per le condizioni complete.

### Segnalazioni

Per segnalare un problema, indica:

- versione di Doze e di Windows;
- passaggi per riprodurlo;
- comportamento atteso e risultato ottenuto;
- eventuale messaggio di errore.

Non includere credenziali o informazioni personali nelle segnalazioni pubbliche.

---

## English

Doze is a Windows utility by **CustomInfo** for scheduling shutdown, restart, sleep and hibernation using a cancellable countdown.

**1.0.0 — First release.**

### Features

- Four actions: shutdown, restart, sleep and hibernation.
- Duration entered using the keyboard, buttons or quick presets.
- Countdown cancellation.
- PC locking and cancellation of shutdowns scheduled through Windows.
- Hibernation status checking, with enabling and disabling on request.
- Italian and English interfaces, with light and dark themes.
- Quick access from the context menu, if selected during installation.

Power operations depend on the PC’s capabilities and Windows settings. Save your work before scheduling shutdown or restart.

### Source code and transparency

This repository contains the complete Doze source code, including its graphical interface.

The code is published so that users can examine how the program works, inspect the operations it requests from Windows and rebuild the project locally.

Publishing source code does not, by itself, certify security or prove that an executable matches that code. Each official package must be associated with the source version from which it was built.

### Structure

```text
src/          Native application and action handling
assets/       Icons, fonts and embedded resources
packaging/    Build scripts, installer and user guide
tests/        Engine tests
project.json  Product information and version
CHANGELOG.md  Version history
LICENSE.txt   Terms of use and source inspection
```

### Building

Doze uses **C++20, Win32 and GDI+**. Building requires Windows and the project’s designated toolchain:

- Zig 0.15.2.
- nlohmann JSON 3.12.0.
- Inno Setup 6.7.3 for creating the installer, subject to its terms of use.

From the repository root, in PowerShell:

```powershell
.\packaging\prepare-toolchain.ps1 -Destination ..\doze-toolchain
```

To build the application and run its tests:

```powershell
.\packaging\build.ps1 `
  -Toolchain ..\doze-toolchain `
  -Output ..\doze-build\Doze `
  -Work ..\doze-build\work `
  -Tests
```

To also create the setup package, add `-Installer` to the same command.

Automated engine and interface tests do not perform real shutdowns, restarts, sleep or hibernation. Testing system actions and installation are separate verification steps.

### Installation and use

The official installer installs Doze for the current Windows user. Adding the **Shift + right-click** context-menu entry is optional.

The bilingual `README.txt` guide included in the installed folder explains how to open and use Doze, change preferences and uninstall it.

### Licence

Doze is free of charge for personal and professional use.

The source code may be viewed, analysed and compiled locally for verification. Reusing the code in other products, distributing modified versions and reusing original graphical assets require the permission specified by the licence.

See [LICENSE.txt](LICENSE.txt) for the complete terms.

### Reporting problems

When reporting a problem, include:

- the Doze and Windows versions;
- steps to reproduce the problem;
- the expected behaviour and actual result;
- any error message.

Do not include credentials or personal information in public reports.
