#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/mman.h>

char *buffer = NULL;                                        //映射指针，因为线程要通过指针的位置来拷贝，所以要定义为全局的
int dfd;                                                    //目标文件的文件描述符，线程要用，所以定义为全局的
double nFinish = 0;                                         //线程已经拷贝了多少文件
double filesize;                                            //源文件的大小
double Percent = 0;                                         //拷贝的百分比
struct argu{                                                //创建线程时给线程传的参数，定义为结构体，里面的两个变量分别为拷贝文件的开始和结束位置
	int start_index;
	int end_index;
};

int Getfilesize(const char* filename)                       //获取文件大小的函数
{                                                          
	int filesize;
	int fd = open(filename,O_RDONLY);
	filesize = lseek(fd,0,SEEK_END);
	return filesize;
}

void* t_job(void* arg)                                      //线程工作函数
{     
	struct argu* temp = (struct argu*)arg;                  
	int nCount1=0;
	int nCount2=0;
	char buf[1024];                                         //缓冲区，一次可以向目标文件中写1024个字节
	int j=temp->start_index;

	lseek(dfd,temp->start_index,SEEK_SET);                  //对目标文件的读写指针进行偏移

	do{                                                    	//拷贝区
		int i=0;
		nCount1 = 0;

		while(j<temp->end_index && nCount1 < 1024){        //通过映射指针将文件写到缓冲区中
			buf[i] = buffer[j];
			i++;
			j++;
			nCount1++;
			nCount2++;
		}

		write(dfd,buf,strlen(buf));                         //向目标文件中写

		nFinish+=strlen(buf);                               //更新nFInish
		Percent = nFinish/filesize*100;                     //计算百分比Percent

		putchar('[');                                       //打印进度条
		for(i=1;i<=100;i++)
			putchar(i<=Percent?'=':' ');
		putchar(']');
		putchar('%');
		printf("%.2lf",Percent);
		fflush(stdout);
		for(i=0;i<108;i++)
			putchar('\b');                                  //打印进度条
		bzero(buf,strlen(buf));                             //清空缓冲区
	}while(temp->end_index-temp->start_index-nCount2>0);

	pthread_exit((void*)1);                                 //线程退出
}

int main(int argc,char* argv[])
{
	if(argc<3 || argc>4){                                                //程序的参数可以为3个或者4个，取决于给没给线程的数量
		printf("参数数量不正确\n");
		exit(1);
	}
	
	int i,j;
	int pthread_num = 0;

	if(argv[3] == NULL){                                                  //如果没给线程的数量，默认为10个线程工作
		pthread_num = 10;
	}else{                                                                //如果给了，就将字符串转化为整型
		for(i=0;argv[3][i]!='\0';i++){
			pthread_num = pthread_num*10+argv[3][i]-'0';
		}
	}

	if(pthread_num<=0 || pthread_num>100){                                //定义线程的数量只能在[1,100]之间
		printf("线程数量不正确\n");
		exit(1);
	}
	
	filesize = Getfilesize(argv[1]);                                      //获取源文件的大小
	int block_size;                                                       //计算出每个线程需要拷贝的大小
	block_size = filesize/pthread_num;
	
	int sfd = open(argv[1],O_RDWR);                                       //打开源文件
	buffer = mmap(NULL,filesize,PROT_READ|PROT_WRITE,MAP_SHARED,sfd,0);   //进行文件映射
	close(sfd);
	dfd = open(argv[2],O_CREAT|O_RDWR,0664);                              //打开目标文件

	int err;
	pthread_t t_arr[pthread_num];                                         //定义tid数组
	struct argu *a[pthread_num];                                          //定义参数数组
	for(i=0;i<pthread_num;i++){                                           //创建线程，设置参数
		a[i] = (struct argu*)malloc(sizeof(struct argu));                
		a[i]->start_index = i*block_size;
		if(i != pthread_num-1){
			a[i]->end_index = (i+1)*block_size;
		}else{                                                           
			a[i]->end_index = filesize;
		}
		if((err = pthread_create(&t_arr[i],NULL,t_job,(void*)a[i]))>0){
			printf("第%d个`线程创建失败%s",i,strerror(err));
			exit(1);
		}
		usleep(66666);
	}

	void *retval;                                                         //线程回收
	for(i=0;i<pthread_num;i++){
		pthread_join(t_arr[i],&retval);
	}
	putchar('\n');
	fflush(stdout);
	return 0;
}




