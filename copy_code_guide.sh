# guide to code is first table and ends with QRCode.py
# strip this out and also put in code directory
echo "# Guide to Code" > gitignore.tmp
echo " " >> gitignore.tmp
ptrn=`grep -n "^[|].*QRCode.py" README.md | sed "s?:.*?:?" | sed "s?.*?^&?"`
num=`grep -n "^[|]" README.md | grep -n "${ptrn}" | sed "s?:.*??"`
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

