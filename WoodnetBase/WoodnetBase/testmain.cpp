#include <iostream>
#include "CommonDefines.h"
#include "StreamQueue.h"

using namespace std;
using namespace woodnet;




int main()
{
	StreamQueue queue(10);

	char str[10] = "";

	queue.write("wow data", 9);

	cout << queue.read(str, 5);
	//str[6] = char("\n");
	cout << " " << str;


	return 0;
}


