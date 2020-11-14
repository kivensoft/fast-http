#include <string.h>
#include "md5.h"

/*md5转换用到的常量，算法本身规定的*/
enum MD5_CONST {
	S11=7, S12=12, S13=17, S14=22, S21=5, S22=9, S23=14, S24=20,
	S31=4, S32=11, S33=16, S34=23, S41=6, S42=10, S43=15, S44=21
};

/** 用于bits填充的缓冲区，为什么要64个字节呢？因为当欲加密的信息的bits数被512除其余数为448时，
 * 需要填充的bits的最大值为512=64*8 。
 */
static uint8_t PADDING[64] = { 0x80 };

static char HEX[] = "0123456789abcdef";

/** 接下来的这几个宏定义是md5算法规定的，就是对信息进行md5加密都要做的运算。
 * 据说有经验的高手跟踪程序时根据这几个特殊的操作就可以断定是不是用的md5
 * F, G, H and I are basic MD5 functions.
 */
#define F(x, y, z) ((x & y) | (~x & z))
#define G(x, y, z) ((x & z) | (y & ~z))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))

/* ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, n) x << n | x >> (32 - n)

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
  Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) { a += F (b, c, d) + x + (uint32_t)ac; a = ROTATE_LEFT (a, s); a += b; }
#define GG(a, b, c, d, x, s, ac) { a += G (b, c, d) + x + (uint32_t)ac; a = ROTATE_LEFT (a, s); a += b; }
#define HH(a, b, c, d, x, s, ac) { a += H (b, c, d) + x + (uint32_t)ac; a = ROTATE_LEFT (a, s); a += b; }
#define II(a, b, c, d, x, s, ac) { a += I (b, c, d) + x + (uint32_t)ac; a = ROTATE_LEFT (a, s); a += b; }

/** encodes input (uint32_t) into output (uint8_t). Assumes len is a multiple of 4.
 * 将4字节的整数copy到字符形式的缓冲区中
 * @param output：用于输出的字符缓冲区
 * @param input：欲转换的四字节的整数形式的数组
 * @param len：output缓冲区的长度，要求是4的整数倍
 */
static void _encode(uint8_t *output, uint32_t *input, size_t len) {
	for (size_t i = 0, j = 0; j < len; i++, j += 4) {
		output[j    ] = (uint8_t)(input[i] & 0xff);
		output[j + 1] = (uint8_t)((input[i] >> 8) & 0xff);
		output[j + 2] = (uint8_t)((input[i] >> 16) & 0xff);
		output[j + 3] = (uint8_t)((input[i] >> 24) & 0xff);
	}
}

/** decodes input (uint8_t) into output (uint32_t). Assumes len is a multiple of 4.
 * 与上面的函数正好相反，这一个把字符形式的缓冲区中的数据copy到4字节的整数中（即以整数形式保存）
 * @param output：保存转换出的整数
 * @param input：欲转换的字符缓冲区
 * @param len：输入的字符缓冲区的长度，要求是4的整数倍
 */
static void _decode(uint32_t *output, uint8_t *input, size_t len) {
	for (size_t i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((uint32_t)input[j]) | (((uint32_t)input[j + 1]) << 8) |
					(((uint32_t)input[j + 2]) << 16) | (((uint32_t)input[j + 3]) << 24);
}

/** MD5 basic transformation. Transforms state based on block.
 * 对512bits信息(即block缓冲区)进行一次处理，每次处理包括四轮
 * @param state[4]：md5结构中的state[4]，用于保存对512bits信息加密的中间结果或者最终结果
 * @param block[64]：欲加密的512bits信息
 */
static void _md5_transform (uint32_t state[4], uint8_t block[64]) {
	uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	_decode(x, block, 64);

	/* Round 1 */
	FF(a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1  */
	FF(d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2  */
	FF(c, d, a, b, x[ 2], S13, 0x242070db); /* 3  */
	FF(b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4  */
	FF(a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5  */
	FF(d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6  */
	FF(c, d, a, b, x[ 6], S13, 0xa8304613); /* 7  */
	FF(b, c, d, a, x[ 7], S14, 0xfd469501); /* 8  */
	FF(a, b, c, d, x[ 8], S11, 0x698098d8); /* 9  */
	FF(d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
	FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

	/* Round 2 */
	GG(a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
	GG(d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
	GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG(a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
	GG(d, a, b, c, x[10], S22, 0x02441453); /* 22 */
	GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG(a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
	GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG(c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
	GG(b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
	GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
	GG(c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
	GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

	/* Round 3 */
	HH(a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
	HH(d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
	HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH(a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
	HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
	HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
	HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH(d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
	HH(c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
	HH(b, c, d, a, x[ 6], S34, 0x04881d05); /* 44 */
	HH(a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
	HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH(b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

	/* Round 4 */
	II(a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
	II(d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
	II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II(b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
	II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II(d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
	II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II(b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
	II(a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
	II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II(c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
	II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II(a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
	II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
	II(b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	/* Zeroize sensitive information. */
	memset(x, 0, sizeof(x));
}

inline static size_t _to_hex(char *dst, const void *src, size_t srclen) {
	for (int i = 0; i < srclen; ++i, dst += 2) {
		uint8_t c = ((uint8_t*)src)[i];
		dst[0] = HEX[c >> 4];
		dst[1] = HEX[c & 15];
	}
	return srclen << 1;
}

void md5_init(md5_ctx_t* md5) {
	memset(md5, 0, sizeof(*md5));
	// Load magic initialization constants.
	// 初始化链接变量，算法要求这样，这个没法解释了
	uint32_t *state = md5->state;
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;
}

void md5_update (md5_ctx_t* md5, const void *input, size_t len) {
	uint32_t i, index, part_len, *count = md5->count, *state = md5->state;
	uint8_t *buffer = md5->buffer;
 
	// Compute number of bytes mod 64
	// 计算已有信息的bits长度的字节数的模64, 64bytes=512bits。
	// 用于判断已有信息加上当前传过来的信息的总长度能不能达到512bits，
	// 如果能够达到则对凑够的512bits进行一次处理
	index = (count[0] >> 3) & 0x3F;
 
	/* Update number of bits *//*更新已有信息的bits长度*/
	if((count[0] += len << 3) < len << 3)
		count[1]++;
	count[1] += len >> 29;
 
	// 计算已有的字节数长度还差多少字节可以 凑成64的整倍数
	part_len = 64 - index;
 
	// Transform as many times as possible.
	// 如果当前输入的字节数 大于 已有字节数长度补足64字节整倍数所差的字节数
	if(len >= part_len) {
		// 用当前输入的内容把md5->buffer的内容补足512bits
		memcpy(&buffer[index], input, part_len);
		// 用基本函数对填充满的512bits（已经保存到md5->buffer中） 做一次转换，转换结果保存到context->state中
		_md5_transform(state, buffer);
 
		// 对当前输入的剩余字节做转换（如果剩余的字节<在输入的input缓冲区中>大于512bits的话 ），
		// 转换结果保存到md5->state中
		// 把i+63<inputlen改为i+64<=inputlen更容易理解
		for(i = part_len; i + 63 < len; i += 64 )
			_md5_transform(state, (uint8_t*)(input + i));
 
		index = 0;
	} else
		i = 0;
 
	// Buffer remaining input
	// 将输入缓冲区中的不足填充满512bits的剩余内容填充到md5->buffer中，留待以后再作处理
	memcpy(&buffer[index], (uint8_t*)(input + i), len - i);
}

/** MD5 finalization. Ends an MD5 message-digest operation, writing the
 * the message digest and zeroizing the context.
 * 获取加密 的最终结果
 * @param this：你前面初始化并填入了信息的md5结构
 * @param digest：保存最终的加密串
 */
void md5_final (md5_ctx_t* md5, uint8_t digest[16]) {
	uint8_t bits[8];
	uint32_t index, pad_len;

	// Save number of bits
	// 将要被转换的信息(所有的)的bits长度拷贝到bits中
	_encode(bits, md5->count, 8);

	// Pad out to 56 mod 64.
	// 计算所有的bits长度的字节数的模64, 64bytes=512bits
	index = (md5->count[0] >> 3) & 0x3f;
	// 计算需要填充的字节数，pad_len的取值范围在1-64之间
	pad_len = (index < 56) ? (56 - index) : (120 - index);
	// 这一次函数调用绝对不会再导致MD5Transform的被调用，因为这一次不会填满512bits
	md5_update(md5, PADDING, pad_len);

	// Append length (before padding)
	// 补上原始信息的bits长度（bits长度固定的用64bits表示），这一次能够恰巧凑够512bits，不会多也不会少
	md5_update(md5, bits, 8);

	// Store state in digest
	// 将最终的结果保存到digest中。ok，终于大功告成了
	_encode(digest, md5->state, 16);

	// Zeroize sensitive information.
	memset(md5, 0, sizeof(*md5));
}

char* md5_string(char dst[33], const void *input, size_t len) {
	uint8_t digest[16];
	md5_ctx_t md5;

	md5_init(&md5);
	md5_update(&md5, input, len);
	md5_final(&md5, digest);
	size_t count = _to_hex(dst, digest, sizeof(digest));
	dst[count] = '\0';
	return dst;
}
