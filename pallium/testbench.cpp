#include <iostream>
#include <iomanip>
#include <time.h>
#include <opencv2/opencv.hpp>
#include <fstream>
#include<thread>
#include <zconf.h>


#define USE_MNIST_LOADER
//#define MNIST_DOUBLE

#include "mnist.h"
#include "topone.h"
#include "opticnerve.h"
#include "neuron.hpp"
#include "testbench.h"
#include "celeb.h"
#include "hzk16.h"
#include "neu.h"
#include "thread_pool.h"
#include "Timer.h"

using namespace std;
using namespace cv;

void conduct(cv::Mat &img,unsigned int x,cv::Mat &nimg);

void mnist2mat(unsigned char *img,cv::Mat& mat) {
	for (int r = 0; r<28; r++)
	{
	    uchar *data = mat.ptr<uchar>(r);
		for (int c = 0; c<28; c++){
		    data[c]=img[r*28+c];
		}
	}
}

Mat AffineTrans(Mat src, Point2f* scrPoints, Point2f* dstPoints)
{
	Mat dst;
	Mat Trans = getAffineTransform(scrPoints, dstPoints);
	warpAffine(src, dst, Trans, Size(src.cols, src.rows), CV_INTER_CUBIC);
	return dst;
}

Mat PerspectiveTrans(Mat src, Point2f* scrPoints, Point2f* dstPoints)
{
	Mat dst;
	Mat Trans = getPerspectiveTransform(scrPoints, dstPoints);
	warpPerspective(src, dst, Trans, Size(src.cols, src.rows), CV_INTER_CUBIC);
	return dst;
}
void get(int d,int &x1,int &y1){
	switch (d){
		case 0:x1=-2,y1=0;break;
		case 1:x1=2,y1=0;break;
		case 2:x1=0,y1=-2;break;
		case 3:x1=0,y1=2;break;
		case 4:x1=-2,y1=-2;break;
		case 5:x1=2,y1=2;break;
		case 6:x1=-2,y1=2;break;
		case 7:x1=2,y1=-2;break;
	}
	x1*=2;
	y1*=2;
}


//test /home/tiansheng/aidata/CelebA
void feel(opticnerve &on,cv::Mat image){
	on.clearfeel();
    int nl = image.rows;
    int nc = image.cols * image.channels();
    int br=(FEELWIDTH-image.rows)/2;
    int bc=(FEELWIDTH-image.cols)/2;

    //遍历图像的每个像素
    for(int j=0; j<nl ;++j)
    {
        uchar *data = image.ptr<uchar>(j);
        for(int i=0; i<nc; ++i)
        {
			on.neuronsdata[i+bc +FEELWIDTH * (j+br)] = data[i];
//		    int div=128;
//            data[i] = data[i]/div*div+div/2;
//			int v=data[i]/div;
//			on.neuronsdata[i+bc +FEELWIDTH * (j+br)+v*FEELWIDTH*FEELWIDTH] = 255;
        }
    }
}


void unfeel(opticnerve &on,cv::Mat &image){
    int nl = image.rows;
    int nc = image.cols * image.channels();

    int begin=(FEELWIDTH-nl)/2;
    //遍历图像的每个像素
    for(int j=0; j<nl ;++j)
    {
        uchar *data = image.ptr<uchar>(j);
        for(int i=0; i<nc; ++i)
        {
        	if(on.neuronsdata[i+begin +FEELWIDTH * (j+begin)+0*FEELWIDTH*FEELWIDTH])
            	data[i] = 64;
			else
				data[i] = 200;
        }
    }
}

void feel_2(opticnerve &on,cv::Mat image){
	on.clearfeel();
    int nl = image.rows;
    int nc = image.cols * image.channels();
    int begin=(FEELWIDTH-nl)/2;
    //遍历图像的每个像素
    for(int j=0; j<nl ;++j)
    {
        uchar *data = image.ptr<uchar>(j);
        for(int i=0; i<nc; ++i)
        {
            //减少图像中颜色总数的关键算法：if div = 64,
			// then the total number of colors is 4x4x4;整数除法时，是向下取整。
		    int div=128;
            data[i] = data[i]/div*div+div/2;
			int v=data[i]/div;
            //printf("%d,%d ",data[i],v);
			//int v=(data[i] +256-20)/256;
			on.neuronsdata[i+begin +FEELWIDTH * (j+begin)+v*FEELWIDTH*FEELWIDTH] = 1;
        }
    }
}

void unfeel2(opticnerve &on,cv::Mat &image){
    int nl = image.rows;
    int nc = image.cols * image.channels();

    int begin=(FEELWIDTH-nl)/2;
    //遍历图像的每个像素
    for(int j=0; j<nl ;++j)
    {
        uchar *data = image.ptr<uchar>(j);
        for(int i=0; i<nc; ++i)
        {
        	if(on.neuronsdata[i+begin +FEELWIDTH * (j+begin)+0*FEELWIDTH*FEELWIDTH])
            	data[i] = 64;
			else
				data[i] = 200;
        }
    }
}

void unfeela(opticnerve &on,cv::Mat &image){
    int nl = image.rows;
    int nc = image.cols * image.channels();

    //遍历图像的每个像素
    for(int j=0; j<nl ;++j)
    {
        uchar *data = image.ptr<uchar>(j);
        for(int i=0; i<nc; ++i)
        {
        	if(on.neuronsdata[i +nc * j+0*nl*nc])
            	data[i] = 0;
			else
				data[i] = 255;
        }
    }
}
void unfeelb(opticnerve &on,cv::Mat &image){
    int nl = image.rows;
    int nc = image.cols * image.channels();

    //遍历图像的每个像素
    for(int j=0; j<nl ;++j)
    {
        uchar *data = image.ptr<uchar>(j);
        for(int i=0; i<nc; ++i)
        {
        	if(on.neuronsdata[i +nc * j+0*nl*nc])
            	data[i] = 255;
			else
				data[i] = 0;
        }
    }
}

void testreappear(opticnerve &on){
	for(KNOWLEDGES::iterator it=on.knowledgs.begin();it!=on.knowledgs.end();it++){
		neuron know =on.neurons[ it->second];
		cv::Mat img(54,44,CV_8U);
		for(int i=0;i<know.inneurons.size();i++){
			on.clearfeel();
			on.reappear(know.inneurons[i]);
			unfeel(on,img);
			imshow("",img);
			waitKey(1000);
		}
		waitKey(1000);
	}
}
void getfocus(){
	opticnerve on(1000000);// s7000.on2: 6071331  12000.on2:10172301
	celebdata data;
	cv::Mat img,result;
	data.loadmat(1,img);
	resize(img,img,Size(img.cols>>3,img.rows>>3),0,0,INTER_LINEAR);//div 8
	feel(on,img);
	on.getfocus();

}
int main_celebA(){
	celebdata data;
	cv::Mat img,result;
	data.loadmat(1,img);
	resize(img,img,Size(img.cols>>3,img.rows>>3),0,0,INTER_LINEAR);//div 8
	//data.saveto("/home/tiansheng/aidata/CelebA/Img/img8d/",1,img);
	//return 1;

	opticnerve on(100000000);// s7000.on2: 6071331  12000.on2:10172301

    double dur;
    clock_t start,ca,end;
    start = clock();

	on.load("celeb8d186000.on2");//6071331
	//on.think();
	//on.save("think8d91000.on2");

//	printf("knows:%d\n",on.knowledgs.size());
//	vector<string> knows;
//	for(int i=on.palliumlayers.size();i>0;i--){
//		printf("layer:%d\n",i);
//		for(int j= on.palliumlayers[i-1]->size();j>0;j--){
//			int n=(*on.palliumlayers[i-1])[j-1];
//			//knows.clear();
//			//on.statusto(n,knows);
//			//if(knows.size()>=10){
//				on.clearfeel();
//				on.reappear(n);
//				//unfeel(on,img);
//				unfeela(on,img);
//				imshow("",img);
//				//printf("\n%dsize(%d): ",n,knows.size());
//				for(int j=0;j<knows.size();j++){
//					printf("%s ",knows[j].data());
//				}
//				waitKey(100);
//			//}
//			//printf("%d:%d ",n,on.neurons[n].tosynapses.size())
//		}
//		waitKey(2000);
//	}

//	on.reduce();
	ca=clock();
	int ok=0,fail=0;
	int beginid=200000;//data.imgs.size()=202599
	for(int i=beginid;i<beginid+100;i++){
		//printf("i=%d \n",i);
	    data.loadmat(i,img);
	    resize(img,img,Size(img.cols>>3,img.rows>>3),0,0,INTER_LINEAR);//div 8
	    //imshow("",img);
		//cv::waitKey(500);

		vector<int> allmax;
		feel(on, img);
		on.calculate(allmax);
		if (data.labels[i] == on.knowidx[allmax[0]]) {
			ok++;
			printf("%d ok  : predict %s ok.\n",i,data.labels[i].data());
			//imshow(data.labels[i],img);
			//waitKey(500);
		}
		else{
			printf("%d fail: %s predict %s\n",i, data.labels[i].c_str(), on.knowidx[allmax[0]].c_str());
			fail++;
		}
		if (allmax.size() > 1) {
			for (int j = 0; j < allmax.size(); j++)
				printf("%s ",  on.knowidx[allmax[j]].c_str());
			printf(" mult predict!\n");
		}
	}
	end = clock();
    dur = (double)(end - ca);
	on.status();
    printf("Use Time test(%d):%10.3fs\n",(ok+fail),(dur/CLOCKS_PER_SEC));
	printf("OK:%d,Fail:%d, score: %0.3f\%\n", ok,fail, ok*100.0 / (ok+fail));


	return 0;

}
void reduce(opticnerve &on){

	vector<int> knows;
	cv::Mat img(28,28,CV_8UC4);
	img.data=(unsigned char*)(on.neuronsdata)+28*28*4;
	for(int i=on.palliumlayers.size();i>0;i--){
		printf("process layer:%d\n",i);
		for(int j=0;j<on.palliumlayers[i-1]->size();){
			int n=(*on.palliumlayers[i-1])[j];
//			int cnt=on.neuthreshold[n];
//			if(cnt>60 && cnt<600){
//    			printf("%d to:%d\n",n,cnt);
//			    on.clearfeel();
//			    on.reappear(n);
//			    imshow("",img);
//			    waitKey(500);
			//}
			//j++;
            //continue;

			knows.clear();
			on.statusto(n,knows);
			//vector<string>::iterator loc=find(knows.begin(),knows.end(),"1");
			//if(loc!=knows.end()){
			if(knows.size()==1 ){
			    //printf("lay:%d id:%d,know:%s\n",i,n,loc->data());
                on.clearfeel();
			    on.reappear(n);
			    imshow("",img);
			    waitKey(200);

			    j++;
			} else{
			    //printf("lay:%d %d:%d\n",i,n,knows.size());
			    j++;
				//std::vector<int>::iterator it = (*palliumlayers[i-1]).begin()+j;
				//(*palliumlayers[i-1]).erase(it);
				//removeneuron(n);
			}
		}
	}

}

int main_mnist(){

    mnist_data *mnist;
	cv::Mat img(28,28,CV_8U);
    unsigned int cnt;
    int ret;
    int i, j;

    if (ret = mnist_load("../../Mnist_data/train-images.idx3-ubyte", "../../Mnist_data/train-labels.idx1-ubyte", &mnist, &cnt))
    {
        printf("An error occured: %d\n", ret);
		free(mnist);
		getchar();
		return 1;
	}
    printf("image count: %d, %d\n", cnt, sizeof(mnist->data));
	opticnerve on(2000000);//94795
	//on.load1("mnist50000.n1");
	//reduce(on);

	double dur;
    clock_t start,ca,end;
    start = clock();

    //goto lbload;

	for (i = 0; i < 2; i++) {
		mnist2mat( (unsigned char*)((mnist+i)->data),img);
		string lb = to_string((mnist+i)->label);
		//imshow("",img);
		//waitKey(0);
		feel(on,img);
		on.remember(lb);
		//on.remember(lb);
//		for(int x=16;x>0;x--){
//			cv::Mat nimg=cv::Mat::zeros(28,28,CV_8U);
//			conduct(img,x,nimg);
//			feel(on,nimg);
//			on.remember(lb);
//		}

//		int x1,y1;
//		Point2f AffinePoints[4] = { Point2f(2, 2), Point2f(2, 25), Point2f(25, 2), Point2f(25, 25) };
//		Point2f AffinePoints1[4] = { Point2f(2, 2), Point2f(2, 25), Point2f(25, 2), Point2f(25, 25)  };
//		for(int d0=0;d0<2;d0++){get(d0,x1,y1);AffinePoints1[0].x=AffinePoints[0].x+x1;AffinePoints1[0].y=AffinePoints[0].y+y1;
//		for(int d1=0;d1<2;d1++){get(d1,x1,y1);AffinePoints1[1].x=AffinePoints[1].x+x1;AffinePoints1[1].y=AffinePoints[1].y+y1;
//		for(int d2=0;d2<2;d2++){get(d2,x1,y1);AffinePoints1[2].x=AffinePoints[2].x+x1;AffinePoints1[2].y=AffinePoints[2].y+y1;
//		for(int d3=0;d3<2;d3++){get(d3,x1,y1);AffinePoints1[3].x=AffinePoints[3].x+x1;AffinePoints1[3].y=AffinePoints[3].y+y1;
//			//Mat dst_affine = AffineTrans(hz, AffinePoints0, AffinePoints1);
//			Mat dst_perspective = PerspectiveTrans(img, AffinePoints, AffinePoints1);
//
//			//imshow("dst", dst_perspective);
//			//waitKey(0);
//			feel(on,dst_perspective);
//			on.remember(lb);
//
//		}
//		}
//		}
//		}

	}
	on.status();
	//on.think();
	//on.save1("mnist10000.n1");
lbload:

	//on.setoutneurons();
    ca=clock();

    int ok = 0,ttl=0;
    char* endc;
	for (i = 0; i < 2; i++) {
		ttl++;
		mnist2mat( (unsigned char*)((mnist+i)->data),img);
		vector<int> allmax;
		feel(on,img);
		on.calculate(allmax);

		int k=allmax[0];
		auto know=on.knowidx.find(k);
		int id = static_cast<int>(strtol(know->second.c_str(),&endc,10));
		if ((mnist + i)->label==id)
			ok++;
	}
	end = clock();
    dur = (double)(end - ca);
    printf("Use Time train:%f, test:%f\n",(double)(ca-start)/CLOCKS_PER_SEC,(dur/CLOCKS_PER_SEC));
	printf("OK:%d, %0.3f\%\n", ok, ok*100.0 / ttl);
	on.status();
	


	free(mnist);
	//getchar();
	return 0;

}

int main_hzk(){

    mnist_data *mnist;
	unsigned int cnt;
    int ret;
    int i, j;
    hzk16 hzk;
    cv::Mat hz(16,16,CV_8U);
    cv::Mat img(28,28,CV_8U);
    //hzk.loadmat('2',hz);
    //imshow("",hz);
    //waitKey(0);
	
    if (ret = mnist_load("../../Mnist_data/train-images.idx3-ubyte", "../../Mnist_data/train-labels.idx1-ubyte", &mnist, &cnt))
    {
        printf("An error occured: %d\n", ret);
		free(mnist);
		getchar();
		return 1;
	}
    printf("image count: %d, %d\n", cnt, sizeof(mnist->data));
	opticnerve on(1000000);//94795

	double dur;
    clock_t start,ca,end;
    start = clock();

	for (i = 0; i < 10; i++) {
		char c='0'+i;
	    hzk.loadmat(c,hz);
	    string lb=&c;
	    //
		//imshow("",hz);
		//waitKey(0);
		feel(on,hz);
		on.remember(lb);

		cv::Mat dst(16,16,CV_8U);
		Point2f AffinePoints[4] = { Point2f(2, 2), Point2f(2, 13), Point2f(13, 2), Point2f(13, 13) };
		Point2f AffinePoints1[4] = { Point2f(2, 2), Point2f(2, 13), Point2f(13, 2), Point2f(13, 13)  };
		int x1,y1;
		for(int d0=0;d0<4;d0++){get(d0,x1,y1);AffinePoints1[0].x=AffinePoints[0].x+x1;AffinePoints1[0].y=AffinePoints[0].y+y1;
		for(int d1=0;d1<4;d1++){get(d1,x1,y1);AffinePoints1[1].x=AffinePoints[1].x+x1;AffinePoints1[1].y=AffinePoints[1].y+y1;
		for(int d2=0;d2<4;d2++){get(d2,x1,y1);AffinePoints1[2].x=AffinePoints[2].x+x1;AffinePoints1[2].y=AffinePoints[2].y+y1;
		for(int d3=0;d3<4;d3++){get(d3,x1,y1);AffinePoints1[3].x=AffinePoints[3].x+x1;AffinePoints1[3].y=AffinePoints[3].y+y1;
			//Mat dst_affine = AffineTrans(hz, AffinePoints0, AffinePoints1);
			Mat dst_perspective = PerspectiveTrans(hz, AffinePoints, AffinePoints1);

			//imshow("dst", dst_perspective);
			//waitKey(0);
			feel(on,dst_perspective);
			on.remember(lb);

		}
		}
		}
		}
	}
	on.status();
	for (i = 0; i < 5000; i++) {
		mnist2mat((unsigned char *) (mnist + i)->data, img);
		resize(img, hz, hz.size());
		feel(on,hz);
		string lb = to_string((mnist + i)->label);
		on.remember(lb);
	}

	on.save("hzk.n2");
lbload:

	//on.setoutneurons();
    ca=clock();

    int ok = 0,ttl=0;
    char* endc;
	for (i = 50000; i < 50500; i++) {
		ttl++;
		mnist2mat((unsigned  char*)(mnist + i)->data,img);
		resize(img,hz,hz.size());
		//string lb = to_string((mnist + i)->label);
        //imshow("",hz);
        //waitKey(0);

		vector<int> allmax;

		feel(on,hz);
		//on.calculate(allmax);
		on.see(allmax);

		int k=allmax[0];
		auto know=on.knowidx.find(k);
		int id = static_cast<int>(strtol(know->second.c_str(),&endc,10));
		if ((mnist + i)->label==id)
			ok++;
	}
	end = clock();
    dur = (double)(end - ca);
    printf("Use Time train:%f, test:%f\n",(double)(ca-start)/CLOCKS_PER_SEC,(dur/CLOCKS_PER_SEC));
	printf("OK:%d, %0.3f\%\n", ok, ok*100.0 / ttl);
	on.status();
	


	free(mnist);
	//getchar();
	return 0;

}
void conduct(cv::Mat &img,unsigned int x,cv::Mat &nimg){
	for (int r = 1; r<27; r++)
	{
	    uchar *data0 = img.ptr<uchar>(r-1);
	    uchar *data1 = img.ptr<uchar>(r);
	    uchar *data2 = img.ptr<uchar>(r+1);
	    uchar *ndata = nimg.ptr<uchar>(r);
		for (int c = 1; c<27; c++){
			unsigned int v=data0[c-1]+data1[c-1]+data2[c-1]+
			   data0[c]+data1[c]+data2[c]+
			   data0[c+1]+data1[c+1]+data2[c+1];
			if(v/9>=x*16 && v/9<x*16+16){
		    	ndata[c]=x*16;
			}
		}
	}

}
cv::Mat img(28,28,CV_8U);
cv::Mat nimg=cv::Mat::zeros(28,28,CV_8U);

brn::brain *pbr;

void neutr(int r,int c){
//    uchar *data0 = img.ptr<uchar>(r-1);
//    uchar *data1 = img.ptr<uchar>(r);
//    uchar *data2 = img.ptr<uchar>(r+1);
//    uchar *ndata = nimg.ptr<uchar>(r);
//
//    while(1){
//        //printf("Wait...%d,%d, id:%d\n",r,c,std::this_thread::get_id());
//        for(int x=16;x>0;x--){
//            usleep(100*1000);
//            unsigned int v=data0[c-1]+data1[c-1]+data2[c-1]+
//               data0[c]+data1[c]+data2[c]+
//               data0[c+1]+data1[c+1]+data2[c+1];
//            if(v/9>=x*16 && v/9<x*16+16){//fire
//                ndata[c]=x*16;
//                pbr->neus[r*28+c].value+=1;
//            }
//        }
//        sleep(1);
//        ndata[c]=0;
//    }
}



void testthread(){
	mnist_data *mnist;
    unsigned int cnt;
    int ret;
    int i, j;

    pbr=new brn::brain();

    mnist_load("../../Mnist_data/train-images.idx3-ubyte", "../../Mnist_data/train-labels.idx1-ubyte", &mnist, &cnt);

    mnist2mat((unsigned char *) ((mnist + 0)->data), img);
    thread *t;
    std::threadpool executor{ 27*20 };
    for(i=1;i<27;i++)
        for(j=1;j<27;j++){
            //t=new thread(neutr,i,j);
            executor.commit(neutr,i,j);
            //br.neus[i*28+j].value=1;
    }
    while(1){
        imshow("",nimg);
        waitKey(100);
        pbr->tm.DetectTimers();
    }
    t->join();
}
int testneurons(){
	mnist_data *mnist;
	cv::Mat img(28,28,CV_8U);
    unsigned int cnt;
    int ret;
    int i, j;
    opticnerve on(2000000);//94795

    mnist_load("../../Mnist_data/train-images.idx3-ubyte", "../../Mnist_data/train-labels.idx1-ubyte", &mnist, &cnt);

	for (i = 0; i < 10; i++) {
		mnist2mat((unsigned char *) ((mnist + i)->data), img);
		string lb = to_string((mnist + i)->label);
		imshow("org",img);
		cv::Mat nimg=cv::Mat::zeros(28,28,CV_8U);
		for(int x=16;x>0;x--){
			conduct(img,x,nimg);
			imshow("new",nimg);
			feel(on,nimg);
			on.remember(lb);
			on.status();
			waitKey(0);
		}
		waitKey(0);
	}
}
void testimg(){
	cv::Mat img1=imread("/home/tiansheng/Pictures/wd1.jpg");
	cv::Mat img2=imread("/home/tiansheng/Pictures/w3.jpg");
	while(1){
		imshow("",img1);
		waitKey(10);
		imshow("",img2);
		waitKey(100);
	}
}
void avgface(){
	celebdata data;
	cv::Mat img;
	data.loadmat(1,img);
	cv::Mat result=cv::Mat::zeros(218,178, CV_8U) ;
	unsigned int tmp[218][178]={0};
	int cnt=0;
	for(int i=0;i<2000;i++){
		data.loadmat(i,img);
		if(img.rows!=0){
			cnt++;
			for(int r=0;r<218;r++)
				for(int c=0;c<178;c++){
				tmp[r][c]+=img.ptr<uchar>(r)[c];
			}
		}
	}
	for(int r=0;r<218;r++)
		for(int c=0;c<178;c++) {
			result.ptr<uchar>(r)[c] = tmp[r][c] / cnt;
		}
	cv::imshow("",result);
	cv::waitKey(0);


}
void testneu(){
	brn::neu n;
	brn::bpneu bpn;
	bpn.Na=150;
	bpn.connect(&n);
	//connect(bpn,n);
    while(1){
        brn::brain::tm.DetectTimers();
        usleep(10*1000);
    }
}

void testbpn(){
	brn::neu n;
	brn::bpneu bpn;
	bpn.Na=40;
	bpn.connect(&n);
	//connect(bpn,n);
    while(1){
        brn::brain::tm.DetectTimers();
        usleep(100*1000);
    }
}

void testbrain(){
	mnist_data *mnist;
    unsigned int cnt;
    int ret;
    int i, j;

    pbr=new brn::brain();

    mnist_load("../../Mnist_data/train-images.idx3-ubyte", "../../Mnist_data/train-labels.idx1-ubyte", &mnist, &cnt);

    mnist2mat((unsigned char *) ((mnist + 1)->data), img);
    for(i=0;i<28;i++)
        for(j=0;j<28;j++){
    	//if(i!=12 and j!=12)
    	// 	continue;
    	uchar *data = img.ptr<uchar>(i);
    	int v=15+data[j]*(150-15)/255;
    	//v=data[j];
    	pbr->bpneus[i][j].Na=v;
    	//printf("i=%2d,j=%2d,data=%d,Na:%3d\n",i,j,data[j],pbr->bpneus[i * 28 + j].Na);
    }
    for(int x=0;x<100;x){
		cv::Mat nimg=cv::Mat::zeros(28,28,CV_8U);
		cv::Mat nimg1=cv::Mat::zeros(28,28,CV_8U);
		cv::Mat nimg2=cv::Mat::zeros(28,28,CV_8U);
        pbr->tm.DetectTimers();
		for(i=0;i<28;i++)
			for(j=0;j<28;j++) {
			    //nimg.ptr<uchar>(i)[j]  = (uchar)(pbr->neus[0][i][j].Na - 15) *255/ (150 - 15) ;
				//nimg1.ptr<uchar>(i)[j] = (uchar)(pbr->neus[1][i][j].Na - 15) *255/ (150 - 15) ;
			    nimg.ptr<uchar>(i)[j]  = (uchar)(pbr->neus[0][i][j].ax->Na );
			    //printf("%d ",(uchar)(pbr->neus[0][i][j].Na ));
				nimg1.ptr<uchar>(i)[j] = (uchar)(pbr->neus[1][i][j].ax->Na );
				nimg2.ptr<uchar>(i)[j] = (uchar)(pbr->neus[2][i][j].ax->Na );
			}
        imshow("a",nimg);
        imshow("b",nimg1);
        imshow("c",nimg2);
        waitKey(1000);
    }

}
int main() {
	//avgface();
    //testthread();
	//testimg();
	//testneurons();
	//main_mnist();
	//main_celebA();
	//main_hzk();
	//getfocus();
	testneu();
	//testbpn();
	//testbrain();
}