# DistMonitor

Monitor rozproszony używający zmodyfikowanego algorytmu Suzuki-Kasami + przykładowe użycie dla problemu producenta-konsumenta.

## Kompilacja

```sh
cmake .
make
```

Testowane na GCC7, CMake 3.9, ZMQ 4.2.2. 

## Uruchomienie przykładu

W pliku `pc.json` zawarta jest przykładowa konfiguracja węzłów w systemie.

Programy uruchamiamy:

```bash
./{producer,consumer} <nr węzła w pliku> <czy jest inicjatorem>
```

Należy zadbać o to, by dokładnie jeden z procesów, obojętnie który, został uruchomiony jako
inicjator.

Przykładowe uruchomienie:

Dla hosta 192.168.0.22:
```bash
./producer 0 true
./consumer 1 false
```

Dla hosta 192.168.0.14:
```bash
./producer 2 false
./consumer 3 false
```