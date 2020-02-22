#include <fstream>
#include "CVideoCamera.h"
#include "CVideoReceiver.h"
#include "CLogUtil.h"
#include "CSDLDisplay.h"
#include <thread>

int main(int argc, char **argv){
	//输出重定向
	std::ofstream lfile("rdbuf.txt");
	std::streambuf *x = std::cout.rdbuf(lfile.rdbuf());
	std::streambuf *r = std::cerr.rdbuf(lfile.rdbuf());

	av_register_all();
	avformat_network_init();
	avcodec_register_all();
	avdevice_register_all();

	CVideoReceiver stuVideoReceiver;
	CSDLDisplay stuSDLDisplay(stuVideoReceiver);
	CVideoCamera stuCamera;

	//开启接收线程
	stuVideoReceiver.Start();
	std::this_thread::sleep_for(std::chrono::microseconds(20));
	//开启发送线程
	stuCamera.Start();

	//主线程进入循环，更新接收到的显示
	stuSDLDisplay.Render();

	stuVideoReceiver.Terminal();
	stuCamera.Terminal();

	//恢复输出重定向
	std::cout.rdbuf(x);
	std::cerr.rdbuf(r);

	return 0;
}
