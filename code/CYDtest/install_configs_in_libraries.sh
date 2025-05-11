d=`echo ~/Documents/Arduino/libraries`

# if there is no saved original file, make one
if ! test -f "${d}/TFT_eSPI/User_Setup_original.h"; then
    mv ${d}/TFT_eSPI/User_Setup.h ${d}/TFT_eSPI/User_Setup_original.h
fi
echo Y | cp -p gitignoreRandomNerdFiles/User_Setup.h ${d}/TFT_eSPI

# curiously the lv_conf.h goes in the directory BELOW the lvgl library
echo Y | cp -p gitignoreRandomNerdFiles/lv_conf.h ${d}

echo "library config files installed in:"
echo "   ${d}/TFT_eSPI/User_Setup.h"
echo "   ${d}/lv_conf.h"
