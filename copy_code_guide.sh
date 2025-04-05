# guide to code is first table and ends with "--- end of table ---"
# strip this out and also put in code directory
echo "# Guide to Code" > gitignore.tmp
echo " " >> gitignore.tmp
# this will store the table into gitignore.tmp
rm xx00 xx01
csplit README.md "%[|] Link [|] Description [|]%" "/[|] [-][-][-] end of table [-][-][-] [|]/"
cat xx00 >> gitignore.tmp
rm xx00 xx01
#
echo "diff of new file follows"
echo "-----------------------------------"
diff -w code/README.md gitignore.tmp
echo "-----------------------------------"
echo " "
echo "enter y to copy to code/README.md"
read resp
if [ "y" = "${resp}" ]; then
    echo "moving to code/README.md"
    mv gitignore.tmp code/README.md
else
    echo "did not move. File is gitignore.tmp"
fi

