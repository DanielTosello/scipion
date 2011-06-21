#include <data/image.h>
#include <iostream>
#include "../../../external/gtest-1.6.0/fused-src/gtest/gtest.h"
// MORE INFO HERE: http://code.google.com/p/googletest/wiki/AdvancedGuide
// This test is named "Size", and belongs to the "MetadataTest"
// test case.
class ImageGenericTest : public ::testing::Test
{
protected:
    //init metadatas
    virtual void SetUp()
    {
#define len 128
        //find binaries directory
        char szTmp[len];
        char pBuf[len];
        sprintf(szTmp, "/proc/%d/exe", getpid());
        int bytes = std::min(readlink(szTmp, pBuf, len), (ssize_t)len - 1);
        if(bytes >= 0)
            pBuf[bytes] = '\0';
        //remove last token
        filename=pBuf;
        filename = filename.removeFilename();
        //get example images/staks
        imageName = filename + "/../applications/tests/test_image/singleImage.spi";
        stackName = filename + "/../applications/tests/test_image/smallStack.stk";
        myImageGeneric.readMapped(imageName);
        myImageGeneric2.readMapped(stackName,1);
    }

    // virtual void TearDown() {}//Destructor
    ImageGeneric myImageGeneric;
    ImageGeneric myImageGeneric2;
    Image<float> myImageFloat;
    FileName imageName;
    FileName stackName;
    FileName filename;

};


TEST_F( ImageGenericTest, equals)
{
    ImageGeneric auxImageG;
    auxImageG.readMapped(imageName);
    ASSERT_TRUE(myImageGeneric==myImageGeneric);
    ASSERT_TRUE(myImageGeneric==auxImageG);
    ASSERT_FALSE(myImageGeneric==myImageGeneric2);
}

TEST_F( ImageGenericTest, copy)
{
    ImageGeneric img1(myImageGeneric);
    ASSERT_TRUE(img1 == myImageGeneric);
    ImageGeneric img2;
    img2 = img1;
    ASSERT_TRUE(img2 == myImageGeneric);
}

// Check if swapped images are read correctly, mapped and unmapped.
TEST_F( ImageGenericTest, readMapSwapFile)
{
    FileName auxFilename(imageName);
    ImageGeneric auxImageGeneric;
    auxFilename=auxFilename.removeExtension((String)"spi");
    auxFilename=auxFilename + "_swap.spi";
    auxImageGeneric.read(auxFilename);
    EXPECT_EQ(myImageGeneric,auxImageGeneric);

    auxImageGeneric.readMapped(auxFilename);
    EXPECT_EQ(myImageGeneric,auxImageGeneric);
}

TEST_F( ImageGenericTest, add)
{
    FileName auxFilename1((String)"1@"+stackName);
    FileName auxFilename2((String)"2@"+stackName);
    ImageGeneric auxImageGeneric1(auxFilename1);
    ImageGeneric auxImageGeneric2(auxFilename2);
    auxImageGeneric1.add(auxImageGeneric2);
    auxFilename2 = filename + "/../applications/tests/test_image_generic/sum.spi";
    auxImageGeneric2.read(auxFilename2);
    EXPECT_TRUE(auxImageGeneric1==auxImageGeneric2);
    auxImageGeneric1.add(auxImageGeneric2);
    EXPECT_FALSE(auxImageGeneric1==auxImageGeneric2);
}

TEST_F( ImageGenericTest, subtract)
{
    FileName sumFn = filename + "/../applications/tests/test_image_generic/sum.spi";
    ImageGeneric sumImg(sumFn);
    FileName fn1((String)"1@"+stackName);
    ImageGeneric img1(fn1);
    sumImg.subtract(img1);
    FileName fn2((String)"2@"+stackName);
    ImageGeneric img2(fn2);

    EXPECT_TRUE(sumImg == fn2);
}

// check if an empty file is correctly created
TEST_F( ImageGenericTest, createEmptyFile)
{
    FileName Fn(filename + "/../applications/tests/test_image_generic/emptyFile.stk");
    const int size = 16;
    createEmptyFile(Fn,size,size,size,size);
    FileName Fn2;
    size_t dump;
    Fn.decompose(dump, Fn2);
    ImageGeneric Img(Fn2);

    int Xdim, Ydim, Zdim;
    size_t Ndim;
    Img.getDimensions(Xdim, Ydim, Zdim, Ndim);
    EXPECT_TRUE( Xdim == size && Ydim == size && Zdim == size && Ndim == size);
    double std, avg, min, max;
    Img().computeStats(avg, std, min, max);
    EXPECT_DOUBLE_EQ(avg, 0);
    EXPECT_DOUBLE_EQ(std, 0);
    EXPECT_DOUBLE_EQ(min, 0);
    EXPECT_DOUBLE_EQ(max, 0);
}

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
