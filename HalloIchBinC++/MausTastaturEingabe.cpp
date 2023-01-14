// HalloIchBinC++.cpp
// Noncompliant or Compliant 

#include <Windows.h> // Eine Bibliothek, die wir brauchen, um die Konsole zu bearbeiten
#include <thread> // Eine Bibliothek, die wir brauchen, um Threads zu erstellen bzw. zu bearbeiten
#include <chrono> // Eine Bibliothek, die wir brauchen, um die Zeit zu messen
#include <iostream> // Eine Bibliothek, die wir brauchen, um die Konsole zu öffnen

// Alle chars werden alle 100 millisekunden einzelnd ausgegeben
void typo(const char* text);

// Diese Funktion leert die Konsole
void clearConsole(HANDLE hStdOut);

int main()
{
	typo("Hallo Welt");
	typo("Ich bin ein Programm, das in C++ geschrieben wurde");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
	typo("Ich wurde nur geschriben, um Tasten/Maus am Bildschirm live zu simulieren");
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	
	// Anstatt ein std array mit INPUT_RECORD 
    // nutzen wir ein INPUT_RECORD array (INPUT_RECORD irInBuf[128] mehr dazu unterhalb), weil obwohl SonarLint meckert, dass wir das nicht tun sollen
    // https://rules.sonarsource.com/cpp/RSPEC-5945
    
	// INPUT_RECORD ist ein Struct, welches die Informationen über die gedrückten Tasten speichert
    // https://learn.microsoft.com/de-DE/windows/console/input-record-str
    INPUT_RECORD ir[128];
	// Hier wird der Handle der Konsole gespeichert von den Eingaben
    // https://learn.microsoft.com/de-DE/windows/console/reading-input-buffer-events
	// Ein HANDLE ist eine Referenz auf ein Objekt im diesen Fall auf den Input
    HANDLE hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	// Hier wird der Handle der Konsole gespeichert was ausgegeben wurde
    HANDLE hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    
	HANDLE hEvent; // Ein Event, welches wir brauchen, um die Eingaben zu lesen
	DWORD nRead; // Die Anzahl der Eingaben, die wir lesen
    
	// Vorherige eingaben werden gelöscht
    clearConsole(hStdOutput);
	// Bei dem Input handle wird der Modus für die Eingaben gesetzt
	// ENABLE_ECHO_INPUT - Gibt an, dass die Eingabe in den Console-Eingabepuffer geschrieben wird. Wenn diese Flag nicht gesetzt ist, wird die Eingabe nicht in den Eingabepuffer geschrieben.
	// ENABLE_LINE_INPUT - Gibt eine reine Zeile an, die vom Benutzer eingegeben wird. Wenn diese Flag nicht gesetzt ist, wird jede Taste, die der Benutzer drückt, als eine Zeile zurückgegeben.
	// ENABLE_MOUSE_INPUT - Um die Maus zu aktivieren. Wenn diese Flag nicht gesetzt ist, wird die Maus nicht aktiviert und nur die Tastatur wird aktiviert.
	// ENABLE_EXTENDED_FLAGS - Um den Edit modus der maus zu deaktivieren. Wenn diese Flag nicht gesetzt ist, ist es möglich die console mit der maus zu makieren.
    // https://learn.microsoft.com/en-us/windows/console/setconsolemode
    SetConsoleMode(hStdInput, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
	// Damit eventuelle vorherige Eingaben gelöscht werden
    // https://learn.microsoft.com/en-us/windows/console/flushconsoleinputbuffer
    FlushConsoleInputBuffer(hStdInput);
    
	// Dieses Event ist zuständig dafür, damit die Console auf Eingaben wartet
    // https://learn.microsoft.com/en-us/windows/win32/sync/using-event-objects
    // https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createeventw
    hEvent = CreateEvent(
        nullptr, // default security attributes (NULL oder 0 also FALSE auch möglich)
		FALSE, // manual-reset event (bedeutet das der Event nicht automatisch zurückgesetzt wird)
		FALSE, // initial state is nonsignaled (bedeutet das der Event nicht automatisch gesetzt wird)
		TEXT("WriteEvent") // object name (ist optional)
    );
    
    // Das Ereignis wird nicht signalisiert (3. Parameter - non-signaled) zu Starten.
	// 1. Parameter handler ( handles(0) ) ist uhrsprünglich ein nicht signaled Ereignis.
	// SonarLint meckert hier, dass wir das nicht tun sollen, aber wir müssen das tun, weil es sonst nicht einfach funktioniert
	HANDLE handles[2] = { hEvent, hStdInput };
    
	// 2. Parameter hat das WaitForMultipleObjects() im griff das auf den Standart input wartet das erlaubt es uns die maus/keyboard Eingaben zu lesen
	// Solange das handle in einem nonsignaled Zustand ist, wird das Programm nicht weiter ausgeführt um ein ineffizientes auslesen zu vermeiden
    // https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects
    while (WaitForMultipleObjects(2, handles, FALSE, INFINITE))
    {
        // Wein irgend ein tastendruck oder maus bewebung erkannt wird, wird das WaitForMultipleObjects TRUE zurückgeben
		// und der Input wird ausgelesen
        // https://learn.microsoft.com/en-us/windows/console/readconsoleinput
        ReadConsoleInput(hStdInput, ir, 128, &nRead);
        for (size_t i = 0; i < nRead; i++)
        {
            // Es filtert nach EventType
            switch (ir[i].EventType)
            {
                case KEY_EVENT:
				    // Wenn [ESCAPE] gedrückt wird wird das Programm beendet
                    if (ir[i].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
					    // mit dem SetEvent() Api funktion wird das hEvent auf den signaled Zustand gesetzt
                        // https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent
                        SetEvent(hEvent);
                    else {
						// Wenn irgendeine andere Taste gedrückt wird, wird die Taste ausgegeben
                        
						// Zeile und Spalte werden auf 0 gesetzt
                        // https://learn.microsoft.com/en-us/windows/console/setconsolecursorposition
                        SetConsoleCursorPosition(hStdOutput, {0, 0});
						// Wir schreiben die gedrückte Taste in die Konsole und WaitForMultipleObjects() wartet wieder auf eine Eingabe
                        printf
                        (
                            "AsciiCode = %d: symbol = %c\n",
                            ir[i].Event.KeyEvent.uChar.AsciiChar,
                            ir[i].Event.KeyEvent.uChar.AsciiChar
                        );                                              
						// Es ist wichtig zu wissen, dass wenn der 3. Parameter von WaitForMultipleObjects() auf FALSE gesetzt wird,
						// wenn das Ereignis auf den signierten Zustand gesetzt wird, wird das Programm nicht weiter ausgeführt
                    }
                    break;
                case MOUSE_EVENT:
					// Zeile wird auf 1 gesetzt und bei der Spalte 0
                    SetConsoleCursorPosition(hStdOutput, {0, 1});
					// Wir schreiben die Mausposition in die Konsole und WaitForMultipleObjects() wartet wieder auf eine Eingabe
                    printf
                    (
                        "%.3d\t%.3d\t%.3d",
                        ir[i].Event.MouseEvent.dwMousePosition.X,
                        ir[i].Event.MouseEvent.dwMousePosition.Y,
						(int)ir[i].Event.MouseEvent.dwButtonState & 0x07 // makiere die ersten 3 bits (0x07 = 0000 0111), was up//output
                    );
                    break;
				default:
					// "We don't care about other events" ^^
					break;
            }
        }
    }
}

void clearConsole(HANDLE hStdOut)
{
    // Es ist wichtig die Consolen größe zu kennen, da wir die Konsole mit einem Zeichen füllen müssen
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    // Hier werden die zellen länge und breite der Konsole gespeichert
    DWORD consoleLength;
    // Hier ist die Anzahl der geschriebenen Zeichen gespeichert
    DWORD writtenWords;
    // DWORD ist ein unsigned int 
    // https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types

    // Um von Buffer die Größe der Konsole zu bekommen, müssen wir die Informationen über die Konsole abrufen
    // https://learn.microsoft.com/en-us/windows/console/console-screen-buffer-info-str
    if (!GetConsoleScreenBufferInfo(hStdOut, &csbi))
    {
        // Wenn es nicht funktioniert, dann wird die Funktion abgebrochen
        return;
    }

    // dwSize ist ein COORD Struct, welches die Länge und Breite der geschriebenen zeichen repräsentiert das in ein DWORD umgewandelt wird

    // COORD ist ein Struct, welches die Koordinaten eines Punktes auf der Konsole repräsentiert 
    // https://learn.microsoft.com/en-us/windows/console/coord-str
    consoleLength = csbi.dwSize.X * csbi.dwSize.Y;

    // Wir füllen die Konsole mit einem Zeichen
    // https://learn.microsoft.com/en-us/windows/console/fillconsoleoutputcharacter
    if (!FillConsoleOutputCharacter(hStdOut, (TCHAR)' ', consoleLength, { 0, 0 } /* Von wo gestartet wird */, &writtenWords /* Warum etwas speichern?*/))
    {
        // Wenn es nicht funktioniert, dann wird die Funktion abgebrochen
        return;
    }

    // Der cursor wird am anfang der Konsole gesetzt
    // https://learn.microsoft.com/en-us/windows/console/setconsolecursorposition
    SetConsoleCursorPosition(hStdOut, { 0, 0 } /* Es ist ein COORD */);
}

void typo(const char* text)
{
	// Wir holen zeichen für zeichen aus dem text bis die strlen erreicht ist
	for (size_t i = 0; i < strlen(text); i++)
	{
		// printf ist hier auch möglich, aber wir wollen die Funktionen von std::cout benutzen
		// Wir schreiben das zeichen in die Konsole
		std::cout << text[i];
		// Wir warten 100ms
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	// Wir gehen eine Zeile nach unten
	std::cout << std::endl;
}