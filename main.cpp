#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <opencv2/opencv.hpp>
using namespace cv;

template <typename T>
inline void LSB_write(MatIterator_<T> ptr, uint8_t bit, int j)
{
    if(bit)
        (*ptr)[j] = (*ptr)[j] | 1;
    else
        (*ptr)[j] = (*ptr)[j] & 0b11111110;
}

template <>
inline void LSB_write<uint8_t>(MatIterator_<uint8_t> ptr, uint8_t bit, int j)
{
    if(bit)
        *ptr = (*ptr) | 1;
    else
        *ptr = (*ptr) & 0b11111110;
}

template <typename T>
inline uint8_t LSB_read(MatIterator_<T> ptr, int j)
{
    return (*ptr)[j] & 1;
}

template <>
inline uint8_t LSB_read<uint8_t>(MatIterator_<uint8_t> ptr, int j)
{
    return (*ptr) & 1;
}


template <typename T> //
Mat writeToImage(Mat image, const std::string& message)
{
    const int usedChannels = (image.channels() == 4) ? 3 : image.channels(); //Alpha channel is not used as it is often homogenic.
    int messageLenght = message.size()*8 + 8;
    int bitCounter = 0;
    MatIterator_<T> end = image.end<T>();
    for(MatIterator_<T> ptr = image.begin<T>(); ptr!=end; ptr++)
    {
        for(int j=0; j<usedChannels; j++)
        {
            uint8_t byte = message[bitCounter/8];
            uint8_t bit = byte & (1 << (bitCounter % 8)); // isolating bit
            LSB_write<T>(ptr, bit, j);
            bitCounter++;
            if(bitCounter >= messageLenght)
                return image;
        }
    }
    return Mat(); // This shouldn't happen. If it did, something is wrong.
}

template <typename TYPE>
std::string readFromImage(Mat& image)
{
    const int usedChannels = (image.channels() == 4) ? 3 : image.channels();
    MatIterator_<TYPE> end = image.end<TYPE>();
    std::vector<uchar> decrypted_message;
    decrypted_message.reserve(256); //it's arbitrary value
    uint8_t byte = 0;
    int bitCounter = 0;
    for(MatIterator_<TYPE> ptr = image.begin<TYPE>(); ptr!=end; ptr++)
    {
        for(int j=0; j<usedChannels; j++)
        {
            uint8_t bit = LSB_read<TYPE>(ptr, j);
            byte = byte | (bit << bitCounter % 8);
            bitCounter++;
            if(bitCounter % 8 == 0)
            {
                if(!byte)
                    return std::string(decrypted_message.begin(), decrypted_message.end());

                decrypted_message.push_back(byte);
                byte = 0;
            }

        }
    }
    return ""; // Probably there is no message in image
}

int check_image_for_errors(Mat& image)
{
    if(image.data == NULL)
    {
        std::cout << "Can't load image. Check path and file permissions." << std::endl;
        return 1;
    } else if(image.depth() != CV_8U || !(image.channels()==1 || image.channels()==3 || image.channels()==4))
    {
        std::cout << "Please use 8-bit RGB(A) or grayscale image." << std::endl;
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if(argc==4) // write mode; argv = {program_name, original_image, attachment, output_image}
    {
        std::string originalImage = argv[1];
        std::ifstream messageFile(argv[2], std::ios::in| std::ios::binary);
        std::string outputPath = argv[3];

        std::string message;
        std::getline(messageFile, message, '\0');

        Mat image = imread(originalImage, IMREAD_UNCHANGED);
        if(check_image_for_errors(image))
            return 1;

        if(!messageFile)
        {
            std::cout << "Can't open message file. Check path and file permissions." << std::endl;
            return 3;
        }
        //Alpha channel is not used as it is often homogenic.
        if(image.rows * image.cols * ((image.channels() == 4) ? 3 : image.channels())  < messageFile.tellg()*8)
        {
            std::cout << "Image is too small to encode given message." << std::endl;
            return 4;
        }
        messageFile.close();

        switch(image.channels())
        {
        case 1:
            image = writeToImage<uint8_t>(image, message);
            break;
        case 3:
            image = writeToImage<Vec3b>(image, message);
            break;
        case 4:
            image = writeToImage<Vec4b>(image, message);
            break;
        }
        if(image.data == NULL)
        {
            std::cout << "Operation failed due to unexpected reason." << std::endl;
            return 1;
        }

        if(imwrite(argv[3], image))
            std::cout << "Operation successfull." << std::endl;

    } else if (argc == 2 && std::string(argv[1]) != "--help" && std::string(argv[1]) != "-h")// read mode; argv = {program_name, image_name}
    {
        Mat image = imread(argv[1], IMREAD_UNCHANGED);
        if(check_image_for_errors(image))
            return 1;

        std::string read_message;
        switch(image.channels())
        {
        case 1:
            read_message = readFromImage<uint8_t>(image);
            break;
        case 3:
            read_message = readFromImage<Vec3b>(image);
            break;
        case 4:
            read_message = readFromImage<Vec4b>(image);
            break;
        }

        if(read_message.empty())
        {
            std::cout << "Image doesn't contain hidden message. If you used hosting service, it may be lost due to compression." << std::endl;
            return 1;
        } else
            std::cout << "Read message: (if you see gibberish there may be no hidden message)\n" << read_message << std::endl;

    }
    else // neither write nor read mode, print help
    {
        std::cout << "Usage:\n"
                     "  Encode message: lsb [original_image] [attachment] [output_image]\n"
                     "  Decode message: lsb [image_name]\n"
                     "  This prompt: lsb [-h or --help]" << std::endl;

    }
    return 0;
}

/* TODO:
Give each error unique return number and make error list
Write proper help prompt
*/
