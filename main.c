#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <zlib.h>
//compile gcc main.c -lpng -std=c99 -O2 -s -Wall -Wextra -o png2raw565
uint8_t * loadpng(char * filename,uint32_t * width,uint32_t * height){
	//loads png as 24 bit
	FILE *fp = fopen(filename, "rb");
	if (!fp){
		perror("Error while reading file:");
		return 0;
	}
	uint8_t header[8];
	fread(header, 1, 8, fp);
	int is_png = !png_sig_cmp(header, 0, 8);
	if (!is_png){
		fclose(fp);
		puts("Make sure what you are loading is a valid png");
		return 0;
	}
	png_structp png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	if (!png_ptr){
		puts("Error creating png read struct");
		return 0;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr){
		puts("Error creating png info struct");
		png_destroy_read_struct(&png_ptr,(png_infopp)NULL, (png_infopp)NULL);
		return 0;
    }
	if (setjmp(png_jmpbuf(png_ptr))){
		puts("libpng setjmp error");
		png_destroy_read_struct(&png_ptr, &info_ptr,NULL);
		fclose(fp);
		return 0;
    }
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);
    *width=png_get_image_width(png_ptr,info_ptr);
    *height=png_get_image_height(png_ptr,info_ptr);
    int color_type=png_get_color_type(png_ptr,info_ptr);
    //Make sure image is 24-bit rgb png
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	uint8_t * picdat=malloc((*width)*(*height)*3);
	png_bytepp row_pointers =malloc(sizeof(png_bytep)*(*height));
	uint32_t y;
	for(y=0;y<*height;++y)
		row_pointers[y]=&picdat[(*width)*y*3];
	png_read_image(png_ptr, row_pointers);
	png_destroy_read_struct(&png_ptr, &info_ptr,(png_infopp)NULL);
	return picdat;
}
int main(int argc,char ** argv){
	if(argc!=3){
		puts("usage ./png2raw565 in.png out.h");
		return 1;
	}
	uint32_t w,h;
	uint8_t * imgin=loadpng(argv[1],&w,&h);
	if(imgin==0){
		puts("Quitting due to error");
		return 1;
	}
	FILE * fo=fopen(argv[2],"w");
	fprintf(fo,"/*FIlename %s\nWidth: %d Height: %d*/\nconst uint16_t %s[] PROGMEM ={\n",argv[1],w,h,argv[1]);
	uint32_t x,y;
	for(y=0;y<h;++y){
		for(x=0;x<w;++x){
			uint16_t rgb;
			rgb=imgin[2]*31/255;
			rgb|=(imgin[1]*63/255)<<5;
			rgb|=(imgin[0]*31/255)<<11;
			fprintf(fo,"%d,",rgb);
			imgin+=3;
		}
		fputc('\n',fo);
	}
	fputs("};",fo);
	fclose(fo);
	imgin-=w*h*3;
	free(imgin);
}
