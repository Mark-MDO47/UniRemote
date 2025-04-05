# guide to code is first table and ends with "--- end of table ---"
# strip this out and also put in code directory
echo "# Guide to Code" > gitignore.tmp
echo " " >> gitignore.tmp
# ptrn_last is regular expression to find line of last line of table
ptrn_last=`grep -n "^[|].*[-][-][-] end of table [-][-][-]" README.md | head -1 | sed "s?:.*?:?" | sed "s?.*?^&?"`
# num is number of lines in table
num=`grep -n "^[| Link | Description |]" README.md | grep -n "${ptrn_last}" | sed "s?:.*??"`
# this will store the table into gitignore.tmp
grep "^[|]" README.md | head -${num} >> gitignore.tmp
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

