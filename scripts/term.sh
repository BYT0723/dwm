Type=$1
case "$Type" in
float)
    st -i -g 90x25+480+200
    ;;
*)
    # st
    alacritty
    ;;
esac
