Write-Host "==========Starting compilation.=========="

flex tma3.l
Write-Host "flex completed..............."

bison -y -d tma3.y
Write-Host "bison completed.............."

gcc -c .\lex.yy.c .\y.tab.c
Write-Host "C compilation of lex and yacc files completed..............."

gcc .\lex.yy.c .\y.tab.c .\symbols.c .\symbol_table.c .\semantic.c .\parser.c .\ast.c .\stack.c .\codegen.c .\isa2.c -o .\tma3.exe
Write-Host "Linking completed. Executable tma3.exe created................"

Write-Host "==========Compilation process finished.=========="