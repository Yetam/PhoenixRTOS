# PhoenixRTOS, Zadanie rekrutacyjne
#### Mateusz Niewiadomski 02/2021
Zadanie rekrutacyjne polegające na dodaniu do obrazu wynikowego programu wypisującego Hello World na konsolę systemu PhoenixRTOS

------------


##### Zmiany dokonane w projekcie

- Do obrazu wynikowego został dodany program *mn_hello* 
- Kod źródłowy oraz Makefile znajdują się w lokalizacji *phoenix-rtos-hello/*
- W pliku *_targets/build.project.ia32-generic *została dodana linia 61 i 62

Program *mn_hello* jest kompilowany razem z całym projektem. Program w wynikowym obrazie systemu znajduje się na filesystemie w lokalizacji */usr/bin/mn_hello*

##### Uruchomienie programu
Po uruchomieniu systemu w QEMU należy wpisać w konsolę:
```bash
(psh)% /usr/bin/mn_hello
```
##### Warunki testowania
Kompilacja i uruchamianie odbywało się na/przy pomocy
- System operacyjny: Linux version 5.8.0-41-generic (buildd@lgw01-amd64-003) (gcc (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0, GNU ld (GNU Binutils for Ubuntu) 2.34) #46~20.04.1-Ubuntu SMP Mon Jan 18 17:52:23 UTC 2021
- Docker version 20.10.2, build 2291f61
- QEMU emulator version 4.2.1 (Debian 1:4.2-3ubuntu6.11)

