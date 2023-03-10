/*
 *	@(#)gallmash.c 2.3 83/09/16 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

unsigned char f_bitmap[240] = {
	0x80, 0x00, 0x08, 0x01, 0x50, 0x82, 0x00, 0x09, 
	0xaa, 0xd0, 0xff, 0xdf, 0x0d, 0xdb, 0xb0, 0xe7, 
	0xff, 0x0d, 0x78, 0x00, 0xfc, 0x3f, 0x0f, 0xc3, 
	0xf0, 0x0f, 0xff, 0x00, 0x45, 0x10, 0x00, 0x0d, 
	0x78, 0x05, 0x00, 0x00, 0x1b, 0x0d, 0x5a, 0xb0, 
	0xf8, 0x1f, 0x0f, 0xc0, 0x30, 0xf9, 0xfd, 0x0f, 
	0xfd, 0xf0, 0xd5, 0x79, 0x0a, 0xf9, 0xf0, 0xf7, 
	0xcf, 0x0f, 0xdb, 0x70, 0xe7, 0xe7, 0x0f, 0x3e, 
	0xf0, 0x06, 0xdb, 0x00, 0x36, 0xd7, 0x87, 0xdf, 
	0x00, 0x15, 0x40, 0x07, 0xdf, 0x0f, 0xbd, 0x50, 
	0x1f, 0xdf, 0x85, 0x6f, 0xb0, 0x73, 0xc7, 0x07, 
	0xc1, 0xf0, 0x78, 0x0f, 0x06, 0xbc, 0xb0, 0x6b, 
	0xc3, 0x07, 0xc7, 0xf0, 0x61, 0x83, 0x06, 0x00, 
	0x30, 0x60, 0x00, 0x7f, 0xef, 0xf0, 0x60, 0x0b, 
	0x07, 0x26, 0xb0, 0x7f, 0xf7, 0x07, 0xc1, 0xf0, 
	0x73, 0x83, 0x07, 0xc1, 0xfe, 0x73, 0xff, 0x07, 
	0x7f, 0x70, 0x70, 0x03, 0x06, 0x00, 0xf0, 0x72, 
	0x69, 0x06, 0x7f, 0x10, 0x7f, 0x7f, 0x07, 0xb0, 
	0x30, 0x7a, 0xaf, 0x0a, 0x00, 0x50, 0xaa, 0xd5, 
	0x0a, 0x00, 0x50, 0xfc, 0x00, 0x00, 0x00, 0x0a, 
	0xeb, 0x00, 0x00, 0x7e, 0x70, 0xf7, 0x8f, 0x00, 
	0x78, 0xf0, 0xe7, 0x8f, 0x00, 0x76, 0xf0, 0xd1, 
	0x83, 0x00, 0x73, 0xfb, 0xf7, 0x03, 0x05, 0x60, 
	0x30, 0x56, 0x01, 0xfe, 0x7f, 0xf0, 0xc0, 0x03, 
	0x00, 0x70, 0x30, 0x07, 0x03, 0x00, 0x78, 0xf0, 
	0x07, 0x8f, 0x18, 0x78, 0xf1, 0x87, 0x83, 0x00, 
	0x7f, 0xf0, 0x2e, 0x0f, 0x00, 0x60, 0x70, 0x07, 
	0x55, 0x00, 0x66, 0x90, 0x07, 0xff, 0x00, 0x75, 
	0x5f, 0x87, 0xff, 0x0d, 0x3c, 0xb0, 0x80, 0x00, 
	0x0d, 0x3c, 0xb0, 0x00, 0xfc, 0x00, 0x00, 0x00, 
	};

unsigned char f_index[844] = {
	0, 1, 0, 1, 0, 2, 0, 3, 
	4, 5, 6, 2, 7, 8, 9, 0, 
	1, 10, 11, 12, 13, 9, 14, 10, 
	15, 4, 13, 7, 16, 1, 0, 17, 
	18, 19, 20, 21, 1, 22, 23, 24, 
	25, 26, 0, 27, 28, 29, 28, 30, 
	14, 31, 32, 33, 26, 34, 35, 36, 
	0, 22, 30, 1, 22, 37, 38, 0, 
	39, 40, 41, 21, 27, 1, 27, 21, 
	41, 40, 39, 0, 42, 37, 43, 22, 
	44, 1, 44, 22, 43, 37, 42, 0, 
	45, 1, 13, 46, 2, 0, 2, 46, 
	13, 1, 45, 0, 1, 47, 1, 0, 
	22, 30, 1, 22, 37, 38, 0, 47, 
	0, 22, 30, 22, 0, 48, 39, 40, 
	21, 1, 22, 37, 42, 49, 0, 27, 
	28, 50, 51, 52, 53, 54, 55, 44, 
	0, 56, 1, 44, 30, 57, 1, 11, 
	0, 55, 16, 26, 58, 39, 40, 21, 
	1, 22, 37, 59, 47, 0, 28, 60, 
	61, 62, 48, 63, 15, 64, 63, 48, 
	62, 65, 16, 55, 0, 40, 41, 66, 
	67, 50, 68, 35, 47, 40, 0, 64, 
	38, 69, 16, 70, 63, 48, 62, 71, 
	52, 10, 0, 27, 22, 37, 42, 49, 
	72, 73, 74, 71, 75, 16, 55, 0, 
	76, 35, 65, 77, 39, 78, 40, 79, 
	21, 56, 1, 80, 0, 45, 50, 52, 
	54, 81, 1, 82, 50, 52, 54, 45, 
	0, 28, 83, 61, 71, 74, 84, 36, 
	48, 39, 40, 27, 85, 0, 22, 30, 
	22, 0, 22, 30, 22, 0, 22, 30, 
	22, 0, 22, 30, 1, 22, 37, 38, 
	0, 48, 86, 27, 30, 87, 30, 27, 
	86, 48, 0, 7, 0, 7, 0, 49, 
	88, 30, 89, 90, 89, 30, 88, 49, 
	0, 45, 10, 91, 92, 39, 40, 21, 
	1, 22, 0, 22, 0, 28, 11, 93, 
	71, 94, 95, 96, 97, 49, 42, 35, 
	98, 0, 1, 82, 99, 50, 100, 11, 
	92, 101, 62, 102, 0, 103, 104, 105, 
	34, 106, 105, 71, 105, 107, 0, 64, 
	108, 109, 69, 49, 69, 59, 110, 28, 
	0, 103, 26, 105, 71, 65, 34, 111, 
	0, 7, 112, 42, 53, 16, 53, 42, 
	59, 47, 0, 7, 112, 42, 53, 16, 
	53, 42, 87, 0, 64, 108, 109, 69, 
	49, 113, 71, 114, 93, 115, 28, 0, 
	116, 71, 47, 71, 116, 0, 10, 1, 
	10, 0, 10, 1, 80, 88, 42, 0, 
	117, 34, 118, 9, 119, 87, 120, 121, 
	122, 123, 26, 124, 125, 0, 87, 42, 
	59, 47, 0, 126, 124, 74, 127, 128, 
	129, 130, 131, 0, 132, 133, 134, 135, 
	136, 137, 138, 139, 140, 141, 142, 62, 
	143, 0, 45, 83, 92, 114, 71, 101, 
	112, 54, 45, 0, 106, 52, 93, 52, 
	144, 42, 87, 0, 45, 83, 92, 114, 
	71, 112, 145, 10, 44, 55, 146, 90, 
	0, 103, 34, 105, 104, 147, 120, 121, 
	122, 123, 26, 124, 125, 0, 76, 93, 
	133, 148, 85, 30, 89, 86, 63, 62, 
	105, 106, 0, 47, 138, 1, 10, 0, 
	125, 133, 75, 11, 10, 0, 149, 65, 
	53, 150, 22, 44, 80, 0, 151, 152, 
	153, 154, 155, 156, 157, 158, 2, 0, 
	125, 133, 112, 159, 54, 81, 1, 82, 
	50, 83, 92, 62, 102, 0, 125, 133, 
	112, 54, 81, 1, 45, 0, 35, 92, 
	39, 40, 21, 1, 22, 37, 160, 35, 
	0, 15, 1, 15, 0, 49, 42, 37, 
	22, 1, 21, 40, 39, 0, 120, 22, 
	120, 0, 80, 44, 161, 162, 105, 0, 
	163, 0, 79, 21, 1, 89, 21, 0, 
	28, 29, 51, 164, 165, 52, 91, 166, 
	0, 69, 49, 167, 49, 72, 73, 74, 
	71, 168, 169, 170, 0, 10, 70, 92, 
	49, 75, 52, 10, 0, 48, 63, 48, 
	171, 172, 61, 71, 74, 173, 174, 0, 
	45, 52, 71, 47, 49, 42, 115, 28, 
	0, 41, 175, 22, 10, 22, 30, 0, 
	176, 172, 105, 162, 177, 49, 7, 35, 
	114, 178, 7, 16, 38, 42, 148, 42, 
	144, 91, 52, 179, 0, 1, 0, 30, 
	1, 10, 0, 39, 0, 164, 39, 92, 
	52, 159, 55, 44, 49, 167, 49, 26, 
	118, 9, 120, 87, 120, 121, 122, 123, 
	180, 0, 30, 1, 10, 0, 181, 182, 
	13, 183, 0, 184, 185, 52, 179, 0, 
	28, 83, 61, 71, 75, 159, 55, 0, 
	186, 187, 124, 71, 65, 188, 147, 49, 
	189, 0, 190, 191, 61, 71, 168, 192, 
	76, 48, 193, 0, 194, 195, 196, 42, 
	87, 0, 60, 52, 112, 88, 30, 89, 
	86, 92, 52, 16, 0, 80, 22, 7, 
	22, 197, 198, 89, 0, 179, 52, 91, 
	36, 0, 125, 133, 112, 54, 81, 1, 
	0, 199, 152, 156, 200, 2, 0, 201, 
	75, 159, 202, 44, 27, 203, 83, 61, 
	204, 0, 116, 133, 112, 54, 81, 1, 
	80, 22, 205, 87, 148, 0, 47, 124, 
	206, 41, 27, 44, 43, 207, 168, 47, 
	0, 86, 21, 40, 21, 27, 21, 40, 
	21, 86, 0, 1, 88, 22, 37, 22, 
	44, 22, 37, 22, 88, 0, 208, 209, 
	210, 211, 212, 0, };

unsigned char f_data_hi[213] = {
	0x00, 0x06, 0x19, 0x03, 0x06, 0x1f, 0x0c, 0x7f, 
	0x33, 0x66, 0x1f, 0x3f, 0x66, 0x66, 0x3e, 0x07, 
	0x3f, 0x38, 0x44, 0x45, 0x39, 0x03, 0x0c, 0x19, 
	0x1a, 0x32, 0x61, 0x07, 0x0f, 0x18, 0x1e, 0x77, 
	0x63, 0x61, 0x61, 0x3f, 0x1e, 0x18, 0x10, 0x00, 
	0x01, 0x03, 0x30, 0x1c, 0x0e, 0x0f, 0x76, 0x7f, 
	0x00, 0x60, 0x11, 0x10, 0x30, 0x30, 0x18, 0x1f, 
	0x02, 0x36, 0x40, 0x30, 0x1f, 0x20, 0x40, 0x00, 
	0x0f, 0x60, 0x05, 0x09, 0x21, 0x20, 0x31, 0x60, 
	0x67, 0x6f, 0x70, 0x70, 0x1f, 0x00, 0x00, 0x01, 
	0x04, 0x0d, 0x0b, 0x11, 0x3f, 0x3c, 0x01, 0x78, 
	0x38, 0x07, 0x01, 0x39, 0x20, 0x30, 0x67, 0x6f, 
	0x6c, 0x67, 0x0f, 0x09, 0x10, 0x20, 0xe0, 0xff, 
	0x60, 0x60, 0x7f, 0xff, 0x10, 0x20, 0x18, 0xfe, 
	0x30, 0x61, 0x20, 0x18, 0xf0, 0xf0, 0x63, 0x6c, 
	0x7c, 0x6e, 0x67, 0x63, 0x60, 0xf0, 0xe0, 0x59, 
	0x4d, 0x4e, 0x44, 0xe4, 0xc0, 0x60, 0x70, 0x78, 
	0x58, 0x4c, 0x46, 0x47, 0x43, 0x41, 0x40, 0xe0, 
	0x37, 0x38, 0x23, 0x7f, 0x70, 0xe0, 0x19, 0xfe, 
	0x66, 0x76, 0x77, 0x33, 0x37, 0x3b, 0x3b, 0x38, 
	0x18, 0x1b, 0x31, 0xff, 0x03, 0x1c, 0x1e, 0xe0, 
	0x70, 0x78, 0x4f, 0x0f, 0x31, 0x39, 0x1e, 0x04, 
	0x1f, 0x3f, 0x40, 0x79, 0xf1, 0xdd, 0x6e, 0xef, 
	0x27, 0x79, 0xef, 0x71, 0x70, 0xf0, 0x0f, 0x11, 
	0x38, 0x00, 0x73, 0x34, 0x38, 0x0c, 0x0e, 0xff, 
	0x3b, 0xf8, 0x1d, 0x0b, 0xf1, 0x08, 0x41, 0x38, 
	0x1c, 0x3e, 0x36, 0x67, 0x43, };

unsigned char f_data_lo[107] = {
	0x00, 0x83, 0x6f, 0xcc, 0x00, 0x8c, 0xe6, 0x0c, 
	0x86, 0xc8, 0x80, 0x0c, 0x22, 0xc0, 0x8c, 0x00, 
	0x6e, 0x8e, 0x60, 0x0c, 0x88, 0x00, 0x00, 0xee, 
	0x60, 0x8c, 0xc8, 0x80, 0x00, 0xc2, 0xce, 0x6e, 
	0xc4, 0x88, 0x80, 0xc6, 0x8c, 0xe4, 0xe4, 0x80, 
	0x00, 0x0c, 0x60, 0xc0, 0x08, 0xec, 0xc6, 0x2a, 
	0xae, 0xe0, 0x84, 0xf0, 0x8c, 0x88, 0x62, 0x40, 
	0x4f, 0x66, 0xfe, 0x00, 0x00, 0x08, 0xe7, 0x76, 
	0x66, 0x6f, 0x72, 0x22, 0x22, 0x22, 0x2a, 0xe3, 
	0x84, 0x90, 0x0e, 0x0f, 0x22, 0x44, 0x4c, 0x88, 
	0x20, 0x8f, 0xcc, 0xe0, 0x6c, 0x86, 0xe6, 0x7c, 
	0x20, 0x2e, 0xec, 0xe7, 0x8c, 0x8c, 0x80, 0x2e, 
	0xef, 0x8c, 0xc2, 0x47, 0x4f, 0x08, 0xf0, 0xc2, 
	0x26, 0xcc, 0x80, };

