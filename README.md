
1. management.exe:
    - creaza directoarele pentru output
    - creaza fisierul mapat in memorie "cssohw3management"
    - lanseaza simultan pay.exe, income.exe si asteapta terminarea lor
    - lanseaza generate.exe si asteapta terminarea sa
    - citeste valoarea din fisierul mapat in memorie si o afiseaza
2. pay.exe si income.exe:
    - parcurge fisierele din directoarele de input - nume de forma yyyy.mm.dd si, pe cate o linie, numere intregi)
    - scrie intr-un fisier comun "logs.txt" operatiile facute (sincronizare facuta cu un mutex)
    - scrie in cate un fisier yyyy.mm.dd_payments/income.txt suma totala pentru fiecare zi, si actualizeaza valoarea din payments/income.txt
    - actualizeaza valoarea din "summary.txt" dupa fiecare parcurgere de fisier (sincronizare facuta cu un semafor)
    - actualizeaza valoarea din fisierul mapat in memorie "cssohw3management" (sincronizare facuta cu un event)
3. generate.exe:
    - afiseaza rezultatele finale si le valideaza
