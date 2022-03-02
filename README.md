# LSB-steganography
It's C++ PoC program to hide text messages in png (tested) and other lossless images files (not tested yet). Message is encoded in least significant bits of pixels. This program uses opencv library.


Usage:</br>
 Encode message: lsb [original_image] [attachment] [output_image]</br>
 Decode message: lsb [image_name]</br>
 Help prompt: lsb [-h or --help]

In future I plan to - among other things - focus on encoding messages while not disturbing entrophy of images.
