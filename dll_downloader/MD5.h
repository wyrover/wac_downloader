#pragma once
#include <string>

#pragma warning(disable : 4786)
using namespace std;

/* MD5 Class. */

class MD5
{
private:

	// this value must initialize first
	unsigned char PADDING[64];

	/** Encodes input (UINT4) into output (unsigned char). Assumes len is
	a multiple of 4.
	*/
	void Encode (unsigned char *output, unsigned long *input, unsigned int len);

	/** Encodes input (UINT4) into output (unsigned char). Assumes len is
	a multiple of 4.
	*/
	void Decode (unsigned long* output, unsigned char *input, unsigned int len);

	/** Decodes input (unsigned char) into output (UINT4). Assumes len is
	a multiple of 4.
	*/
	void MD5_memset (unsigned char* output, int value, unsigned int len);

	// Note: Replace “for loop” with standard memcpy if possible.
	void MD5_memcpy (unsigned char* output, unsigned char* input, unsigned int len);

public:

	// MD5 context.
	struct MD5_CTX
	{
		unsigned long state[4]; // state (ABCD)
		unsigned long count[2]; // number of bits, modulo 2^64 (lsb first)
		unsigned char buffer[64]; // input buffer
	};

	MD5();

	virtual ~MD5();

	/** step 1
	MD5 initialization. Begins an MD5 operation, writing a new context.
	*/
	void MD5Init (struct MD5_CTX *context);

	/** step 2
	MD5 block update operation. Continues an MD5 message-digest
	operation, processing another message block, and updating the
	context.
	*/
	void MD5Update (struct MD5_CTX *context /* context */,
		unsigned char *input /* input block */,
		unsigned int inputLen /* length of input block */);

	/** step 3
	MD5 basic transformation. Transforms state based on block.
	*/
private : void MD5Transform (unsigned long state[4], unsigned char block[64]);

		  /** step 4
		  MD5 finalization. Ends an MD5 message-digest operation, writing the
		  the message digest and zeroizing the context.
		  */
public : void MD5Final (unsigned char digest[16]/* message digest */, MD5_CTX *context /* context */);

		 string MD5String(unsigned char *pData, unsigned long DataLen, bool bUppercase = true);

		 // 将寄存器A, B, C, D的值转换为十六进制, 并以字符串方式返回
		 string ResultToHex(unsigned char digest[16], bool bUppercase = true);

private:

	string m_strErr; // Error Description

	/** 获取字符串的MD5值
	pData - 待计算MD5值的数据
	DataLen - 数据长度(单位, 字节)
	Uppercase - 结果是否转换为大写
	*/

	// 此函数可提前结束MD5File函数的执行
	void StopCalculate();

	// Get Error Description
	string GetError();

};