
OUTPUT=~/tsmuxer-pkg

mkdir $OUTPUT
cp ./converter $OUTPUT/
cp ./ffmpeg $OUTPUT/
strip $OUTPUT/converter
strip $OUTPUT/ffmpeg
chmod +x $OUTPUT/converter
chmod +x $OUTPUT/ffmpeg
