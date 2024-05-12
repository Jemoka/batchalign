@echo on
dir
pip install .
cd baln\opt\CLANUtils\src
make
cd ..\unix
mkdir %PREFIX%\bin
dir
move bin\* %PREFIX%\bin
