#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <png.h>
#include <zlib.h>
//compile gcc main.c -lpng -O2 -s -Wall -Wextra -o png2raw
static uint8_t * loadpng(char * filename,uint32_t * width,uint32_t * height){
	//loads png as 24 bit
	FILE *fp = fopen(filename, "rb");
	if (!fp){
		perror("Error while reading file:");
		return 0;
	}
	uint8_t header[8];
	if(fread(header, 1, 8, fp)!=8){
		perror("Cannot read 8 bytes");
		fclose(fp);
		return 0;
	}
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
char * lastStrstr(const char * haystack,const char * needle){
	char*temp=haystack,*before=0;
	while(temp=strstr(temp,needle)) before=temp++;
	return before;
}
int main(int argc,char ** argv){
	if(argc<3){
		printf("usage %s [-8] in.png out.h\nThe -8 skips the rgb565 conversion and rights rgb888 instead",argv[0]);
		return 1;
	}
	uint32_t w,h;
	uint8_t * imgin=loadpng(argv[argc-2],&w,&h);
	if(imgin==0){
		puts("Quitting due to error");
		return 1;
	}
	int use32=0;
	int arg;
	for(arg=1;arg<argc-2;++arg){
		if(!strcmp("-8",argv[arg]))
			use32=1;
	}
	FILE * fo=fopen(argv[argc-1],"w");
	char * fname=alloca(strlen(argv[argc-2])+1);
	strcpy(fname,argv[argc-2]);
	char*removePng=lastStrstr(fname,".png");
	if(removePng)
		*removePng=0;
	fprintf(fo,"/*Filename %s\nWidth: %d Height: %d*/\nconst uint%d_t %s[] ={\n",argv[argc-2],w,h,use32?8:16,fname);
	uint32_t x,y;
	for(y=0;y<h;++y){
		for(x=0;x<w;++x){
			if(use32){
				fprintf(fo,"%d,%d,%d,",imgin[0],imgin[1],imgin[2]);
				imgin+=3;
			}else{
				uint16_t rgb;
				rgb=imgin[2]*31/255;
				rgb|=(imgin[1]*63/255)<<5;
				rgb|=(imgin[0]*31/255)<<11;
				fprintf(fo,"%d,",rgb);
				imgin+=3;
			}
		}
		fputc('\n',fo);
	}
	fputs("};",fo);
	fclose(fo);
	imgin-=w*h*3;
	free(imgin);
	return 0;
}
