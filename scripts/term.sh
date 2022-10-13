Type=$1
case "$Type" in
float)
  st -i -g 80x25+500+200
  ;;

translator)
  st -i -g 80x20+500+200 -e ~/.dwm/translate.sh
  ;;
*)
  st
  ;;
esac
