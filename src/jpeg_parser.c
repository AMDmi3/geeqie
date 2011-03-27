
#include "main.h" 
#include "jpeg_parser.h"

gboolean jpeg_segment_find(guchar *data, guint size,
			    guchar app_marker, const gchar *magic, guint magic_len,
			    guint *seg_offset, guint *seg_length)
{
	guchar marker = 0;
	guint offset = 0;
	guint length = 0;

	while (marker != JPEG_MARKER_EOI)
		{
		offset += length;
		length = 2;

		if (offset + 2 >= size ||
		    data[offset] != JPEG_MARKER) return FALSE;

		marker = data[offset + 1];
		if (marker != JPEG_MARKER_SOI &&
		    marker != JPEG_MARKER_EOI)
			{
			if (offset + 4 >= size) return FALSE;
			length += ((guint)data[offset + 2] << 8) + data[offset + 3];

			if (marker == app_marker &&
			    offset + length < size &&
			    length >= 4 + magic_len &&
			    memcmp(data + offset + 4, magic, magic_len) == 0)
				{
				*seg_offset = offset + 4;
				*seg_length = length - 4;
				return TRUE;
				}
			}
		}
	return FALSE;
}


typedef enum {
	TIFF_BYTE_ORDER_INTEL,
	TIFF_BYTE_ORDER_MOTOROLA
} TiffByteOrder;

#define TIFF_TIFD_OFFSET_TAG 0
#define TIFF_TIFD_OFFSET_FORMAT 2
#define TIFF_TIFD_OFFSET_COUNT 4
#define TIFF_TIFD_OFFSET_DATA 8
#define TIFF_TIFD_SIZE 12



guint16 tiff_byte_get_int16(guchar *f, TiffByteOrder bo)
{
	guint16 align_buf;

	memcpy(&align_buf, f, sizeof(guint16));

	if (bo == TIFF_BYTE_ORDER_INTEL)
		return GUINT16_FROM_LE(align_buf);
	else
		return GUINT16_FROM_BE(align_buf);
}

guint32 tiff_byte_get_int32(guchar *f, TiffByteOrder bo)
{
	guint32 align_buf;

	memcpy(&align_buf, f, sizeof(guint32));

	if (bo == TIFF_BYTE_ORDER_INTEL)
		return GUINT32_FROM_LE(align_buf);
	else
		return GUINT32_FROM_BE(align_buf);
}

void tiff_byte_put_int16(guchar *f, guint16 n, TiffByteOrder bo)
{
	guint16 align_buf;

	if (bo == TIFF_BYTE_ORDER_INTEL)
		{
		align_buf = GUINT16_TO_LE(n);
		}
	else
		{
		align_buf = GUINT16_TO_BE(n);
		}

	memcpy(f, &align_buf, sizeof(guint16));
}

void tiff_byte_put_int32(guchar *f, guint32 n, TiffByteOrder bo)
{
	guint32 align_buf;

	if (bo == TIFF_BYTE_ORDER_INTEL)
		{
		align_buf = GUINT32_TO_LE(n);
		}
	else
		{
		align_buf = GUINT32_TO_BE(n);
		}

	memcpy(f, &align_buf, sizeof(guint32));
}

gint tiff_directory_offset(guchar *data, const guint len,
				guint *offset, TiffByteOrder *bo)
{
	if (len < 8) return FALSE;

	if (memcmp(data, "II", 2) == 0)
		{
		*bo = TIFF_BYTE_ORDER_INTEL;
		}
	else if (memcmp(data, "MM", 2) == 0)
		{
		*bo = TIFF_BYTE_ORDER_MOTOROLA;
		}
	else
		{
		return FALSE;
		}

	if (tiff_byte_get_int16(data + 2, *bo) != 0x002A)
		{
		return FALSE;
		}

	*offset = tiff_byte_get_int32(data + 4, *bo);

	return (*offset < len);
}

typedef gint (* FuncParseIFDEntry)(guchar *tiff, guint offset,
				 guint size, TiffByteOrder bo,
				 gpointer data);


gint tiff_parse_IFD_table(guchar *tiff, guint offset,
			  guint size, TiffByteOrder bo,
			  guint *next_offset,
			  FuncParseIFDEntry parse_entry, gpointer data)
{
	guint count;
	guint i;
	guint next;


	/* We should be able to read number of entries in IFD0) */
	if (size < offset + 2) return -1;

	count = tiff_byte_get_int16(tiff + offset, bo);
	offset += 2;
printf("count %d\n", count);
	/* Entries and next IFD offset must be readable */
	if (size < offset + count * TIFF_TIFD_SIZE + 4) return -1;

	for (i = 0; i < count; i++)
		{
		parse_entry(tiff, offset + i * TIFF_TIFD_SIZE, size, bo, data);
		}
	
	next = tiff_byte_get_int32(tiff + offset + count * TIFF_TIFD_SIZE, bo);
printf("next %d\n", next);
	if (next_offset) *next_offset = next;
	
	return 0;
}

static gint mpo_parse_Index_IFD_entry(guchar *tiff, guint offset,
				 guint size, TiffByteOrder bo,
				 gpointer data)
{
	guint tag;
	guint format;
	guint count;
	guint data_val;
	guint data_offset;
	guint data_length;

	MPOData *mpo = data;

	tag = tiff_byte_get_int16(tiff + offset + TIFF_TIFD_OFFSET_TAG, bo);
	format = tiff_byte_get_int16(tiff + offset + TIFF_TIFD_OFFSET_FORMAT, bo);
	count = tiff_byte_get_int32(tiff + offset + TIFF_TIFD_OFFSET_COUNT, bo);
	data_val = tiff_byte_get_int32(tiff + offset + TIFF_TIFD_OFFSET_DATA, bo);
printf("tag %x format %x count %x data_val %x\n", tag, format, count, data_val);

        if (tag == 0xb000)
        	{
        	mpo->version = data_val;
        	}
        else if (tag == 0xb001)
        	{
        	mpo->num_images = data_val;
        	}
	else if (tag == 0xb002)
		{
		guint i;
		data_offset = data_val;
		data_length = count;
		if (size < data_offset || size < data_offset + data_length)
			{
			return -1;
			}
		if (count != mpo->num_images * 16)
			{
			return -1;
			}
		
		mpo->images = g_new0(MPOEntry, mpo->num_images);
			
		for (i = 0; i < mpo->num_images; i++) {
			guint image_attr = tiff_byte_get_int32(tiff + data_offset + i * 16, bo);
			mpo->images[i].type_code = image_attr & 0xffffff;
			mpo->images[i].representative = !!(image_attr & 0x20000000);
			mpo->images[i].dependent_child = !!(image_attr & 0x40000000);
			mpo->images[i].dependent_parent = !!(image_attr & 0x80000000);
			mpo->images[i].length = tiff_byte_get_int32(tiff + data_offset + i * 16 + 4, bo);
			mpo->images[i].offset = tiff_byte_get_int32(tiff + data_offset + i * 16 + 8, bo);
			mpo->images[i].dep1 = tiff_byte_get_int16(tiff + data_offset + i * 16 + 12, bo);
			mpo->images[i].dep2 = tiff_byte_get_int16(tiff + data_offset + i * 16 + 14, bo);
			
			if (i == 0) 
				{
				mpo->images[i].offset = 0;
				}
			else
				{
			 	mpo->images[i].offset += mpo->mpo_offset;
			 	}
			 	
			printf("img %x %x %x\n", image_attr, mpo->images[i].length, mpo->images[i].offset);
			}
		}

	return 0;
}

MPOData *jpeg_get_mpo_data(guchar *data, guint size)
{
	guint seg_offset;
	guint seg_size;
	if (jpeg_segment_find(data, size, JPEG_MARKER_APP2, "MPF\x00", 4, &seg_offset, &seg_size) && seg_size >16)
		{
		guint offset;
		TiffByteOrder bo;
		MPOData *mpo;
		guint i;

		printf("mpo signature found at %x\n", seg_offset); 
		data += seg_offset + 4;
		seg_size -= 4;
		
		if (!tiff_directory_offset(data, seg_size, &offset, &bo)) return NULL;

		mpo = g_new0(MPOData, 1);
		mpo->mpo_offset = seg_offset + 4;
		
		tiff_parse_IFD_table(data,  offset , seg_size, bo, NULL, mpo_parse_Index_IFD_entry, (gpointer)mpo);
		if (!mpo->images) mpo->num_images = 0;
		
		for (i = 0; i < mpo->num_images; i++)
			{
			if (mpo->images[i].offset + mpo->images[i].length > size)
				{
				mpo->num_images = i;
				DEBUG_1("MPO file truncated to %d valid images", i);
				break;
				}
			}
		return mpo;
		}
	return NULL;
}

void jpeg_mpo_data_free(MPOData *mpo)
{
	if (mpo)
		{
		if (mpo->images) g_free(mpo->images);
		g_free(mpo);
		}
}