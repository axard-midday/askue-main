# 
# Сборка для askue-main
#

project_name := askue-main
source_dirs := ../src
dest_dir := ../bin

source_files := $(wildcard $(addsuffix /*.c, $(source_dirs)))

object_files := $(notdir $(source_files))
object_files := $(object_files:.c=.o)

link_flags := -lsqlite3 -lconfig -laskue
compile_flags := -Wall -std=c99 -D_ASKUE_DEBUG

CC := gcc-3.4

output := $(project_name)

all: $(project_name) Clean_Object_Files Move_BinDir Strip_Elf 

Move_BinDir:
	mv $(output) $(addsuffix $(output), $(addsuffix /, $(dest_dir)))

Clean_Object_Files:
	rm *.o
	
Strip_Elf:
	strip -s $(addsuffix $(output), $(addsuffix /, $(dest_dir)))
	
$(project_name): $(object_files)
	$(CC) $^ -o $(output) -pipe $(link_flags)

%.o:%.c 
	$(CC) $^ -c $(compile_flags) $(addprefix -I, $(source_dirs)) -pipe 
	
VPATH := $(source_dirs)

.PHONY: all Clean_Object_Files Move_BinDir Strip_Elf
