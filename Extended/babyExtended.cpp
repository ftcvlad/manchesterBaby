//Author: Can Gafuroglu, Erik Jeny, Vladislav Voicehovics, Radu Birgauan
//Purpose: To provide the user with a simulation of the Manchester Baby computer. Part of an assignment submission for the AC21009 module at the University of Dundee.
//Date: 10/11/2015

//to be compiled with g++ options:    -L/usr/X11R6/lib -lm -lpthread -lX11 -std=c++11

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <bitset>
#include <algorithm>

//include CImg graphics library:
#include "CImg.h"

using namespace std;
using namespace cimg_library;

//forward declaration of member functions:

void clearConsole();
void displayWelcome();
void renderSimulation();
bool wasMousePressed();

int mouseStatePrev=0; //previous mouse state
int mouseStateCur=0; //current mouse state

/***** MANCHESTER BABY SIMULATOR CLASS CODE*** */

class coreMB{

private:

	
	bitset<32> acc; //Accumulator
	bool isStopped;
	int loc; //Number of memory locations
	bool carry; //Carry flag

public:

	bitset<32> ci, pi; //Control Instruction (Program counter) and Present Instruction (Instruction decode register)
	vector<bitset<32>> mem; //Expandable memory
	vector<bitset<32>> memInitialState;

	//Constructor

	coreMB(){

		mem = vector<bitset<32>>(32);
		memInitialState = vector<bitset<32>>(32);

		loc = 5;
		carry=false;
		isStopped=false;
	}

	/**********************Simulator function***********************/

	int getLoc(){
		return loc;
	}

	bool getCarry(){
		return carry;
	}

	bool getStopped(){
		return isStopped;
	}

	void setCarry(bool a){
		carry = a;
	}


	void modMem(int amount){ //Sets the amount of memory locations to the set amount!!*!

		int newSize = mem.size()+amount;
		mem.resize(newSize);
		loc = 0;

		while(newSize!=0){
			newSize = newSize/2;
			loc++;
		}
	}


	int adrGen(bitset<32>& a){ //Takes the first 12 (or fewer) address bits and turns them into an integer

		int adr = 0;
		for(int i=31; i > (31-loc); i--){
			adr = adr + ( (int)a.test(i) )*( pow(2, (31-i)) ); 
		}


		if (a.test(15)){//15(!) not 16
			int indAddr=0;
			for(int i=14; i > (14-loc); i--){
				indAddr = indAddr + ( (int)a.test(i) )*( pow(2, (14-i)) );
			}

			adr = adr+numGen(mem[indAddr]) + 1;

		}

		return adr;
	}


	int numGen(bitset<32> a){ //Takes bitset and turns them into a signed integer

		int num = 0;
		bool sig = false;

		if(a.test(0)==true){ //Checks if it's negative, and if negative executes it

			sig=true;
			a = complement2(a);
		}


		for(int i=31; i > 0; i--){

			num = num + ( (int)a.test(i) )*( pow(2, (31-i)) ); 
		}

		if(sig)
			return -num;
		else
			return num;
	}

	bitset<32> bitGen(int num){ //Generates a bitset out of a signed integer

		bitset<32> temp;
		int i = 31;

		if(num<0){ //Checks if it's negative, and if negative executes it

			num = -num;

			while(num!=0){
			
				temp.set(i, (bool)(num%2));
				num = num/2;
				i--;

				if(i==0){
					break;
				}
			}

			 temp = complement2(temp);

		}

		else{ //Executes it if it's positive

			while(num!=0){
			
				temp.set(i, (bool)(num%2));
				num = num/2;
				i--;

				if(i==0){
					break;
				}
			}
		}

		return temp;
	}

	/***********************Control function************************/

	void coreReset(bool emptyMe){

		if (emptyMe){
			for(unsigned int i=0; i<mem.size(); i++){
				mem[i].reset();
			}
		}
		else{
			mem = memInitialState;
		}


		ci.reset();
		pi.reset();
		acc.reset();
		carry=false;
		isStopped=false;
	}

	/*********************Arithmetic-Logic Unit***********************/

	bool hAdd(bool a, bool b){ //Half Adder

		if(carry){
			if(a==true){
				if(b==true){ //1 + 1 + 1 
					carry=true;
					return true;
				}
				else{ //1 + 0 + 1
					carry=true;
					return false;
				}
			}

			else{
				if(b==true){ //0 + 1 + 1
					carry=true;
					return false;
				}

				else{ //0 + 0 + 1
					carry=false;
					return true;
				}
			}
		}

		else{
			if(a==true){
				if(b==true){ //1 + 1 + 0 
					carry=true;
					return false;
				}
				else{ //1 + 0 + 0
					carry=false;
					return true;
				}
			}
			else{
				if(b==true){ //0 + 1 + 0
					carry=false;
					return true;
				}
				else{ //0 + 0 + 0
					carry=false;
					return false;
				}
			}
		}	


	}

	void fAdd(bitset<32> a){ //Full Adder

		for(int i=31; i>=0; i--){

			acc.set(i, hAdd(a.test(i), acc.test(i)));
		}
		carry = false;

	}

	void fAddBitset(bitset<32>& a, bitset<32> &b){ //Full Adder, overloaded. For working on a bitset b (not for accumulator)

		for(int i=31; i>=0; i--){
			b.set(i, hAdd(a.test(i), b.test(i)));
		}
		carry=false;
	}

	void inc(bitset<32> &a){ //Increment by One. 

		a = bitGen(adrGen(a)+1);
	}

	bitset<32>  complement2(bitset<32> a){ //Two's Complement
		a.flip();
		bitset<32> t("10000000000000000000000000000000");
		fAddBitset(t, a);
		return a;
	}

	bool comp(){ //Tests if there's a negative number in the Accumulator

		if(acc.test(0)==true)
			return true;
		else
			return false;

	}

	/* SECTION: UNUSED LOGIC CODE

	bool boolAND(bool a, bool b){ //Logical AND

		if(a==true)
			if(b==true)
				b=true; //B was already true
			else
				b=false; //B was already false
		else
			if(b==true)
				b=false;
			else
				b=false; //B was already false
		

		return b;

	}

	bool boolOR(bool a, bool b){ //Logical OR

		if(a==true)
			if(b==true)
				b=true; //B was already true
			else
				b=true;
		else
			if(b==true)
				b=true; //B was already true
			else
				b=false; //B was already false
		

		return b;

	}

	bool boolNOT(bool a){ //Logical NOT

		a = !a;
		return a;

	}

	bool boolXOR(bool a, bool b){ //Logical XOR

		if(a==true)
			if(b==true)
				b=false;
			else
				b=true;
		else
			if(b==true)
				b=true; //B was already true
			else
				b=false; //B was already false
		
		return b;

	}

	void bitAND(bitset<32> a){ //ANDs a bitset and the accumulator

		for(int i=31; i>0; i--){

			acc.set(i, boolAND(a.test(i), acc.test(i)));
		}
	}

	void bitOR(bitset<32> a){ //ORs a bitset and the accumulator

		for(int i=31; i>0; i--){

			acc.set(i, boolOR(a.test(i), acc.test(i)));
		}
	}

	void bitXOR(bitset<32> a){ //XORs a bitset and the accumulator

		for(int i=31; i>0; i--){

			acc.set(i, boolXOR(a.test(i), acc.test(i)));
		}
	}

	

	void bitNOT(){ //NOTs the accumulator

		acc.flip();		
	}

	END OF UNUSED LOGIC CODE*/

	/****************************Fetching*****************************/

	void loadMem(){ //Loads from memory location into accumulator
		//load store word into accumulator
		acc = mem[adrGen(pi)];
	}

	void loadMemNegate(){ //Loads from memory location into accumulator (-)
		//load store word into accumulator
		acc = mem[adrGen(pi)];
		//take its Two's complement negative
		acc = complement2(acc);
	}

	void storeMem(){ //Stores from accumulator into memory location

		mem[adrGen(pi)] = acc;
	}

	/*void setACC(bitset<32> a){ //Sets Accumulator

		acc = a;
	}*/

	bitset<32> getACC(){ //Returns Accumulator

		return acc;
	}

	void incCI(){ //Increments Control Instruction

		if (!isStopped)
		{
			inc(ci);
		}
		
	}

	void loadIns(){  //Loads instruction into 

		pi = mem[adrGen(ci)];
	}

	/**********************Instruction Decoding**********************/

	void decIns(){ //Decodes instruction and deals with it

		//decode and execute instructions

		//bits 13, 14, 15 and 12 used (12 for additional instructions)

		//0111 - STOP
		if (pi.test(18)==true && pi.test(17)==true && pi.test(16)==true && pi.test(19)== false ) 
		{ 
			isStopped=true; //111 - STOP (STP)
			cout << "STOP." << endl;
		}

		//0011 - COMPARE (CMP)
		if (pi.test(18)==false && pi.test(17)==true && pi.test(16)==true && pi.test(19)== false ) 
		{
			if(comp())
			{
				inc(ci);
				cout << "COMPARE: Incremented CI." << endl;
			}
			else
			{
				cout << "COMPARE: NOT incrementing CI." << endl;
			}
		}

		//0101 - SUBTRACT
		if (pi.test(18)==true && pi.test(17)==false && pi.test(16)==true && pi.test(19)== false ) 
		{
			fAdd(complement2(mem[adrGen(pi)]));
			cout << "SUBTRACT." << endl;
		}

		//0001 - SUBTRACT (Alternative)
		if (pi.test(18)==false && pi.test(17)==false && pi.test(16)==true && pi.test(19)== false ) 
		{
			fAdd(complement2(mem[adrGen(pi)]));
			cout << "SUBTRACT." << endl;
		}

		//0110 - STORE
		if (pi.test(18)==true && pi.test(17)==true && pi.test(16)==false && pi.test(19)== false ) 
		{
			storeMem();
			cout << "STORE." << endl;
		}

		//0010 - LDN (LOAD AND NEGATE)
		if (pi.test(18)==false && pi.test(17)==true && pi.test(16)==false && pi.test(19)== false ) 
		{
			loadMemNegate();
			cout << "LDN. (Load and Negate)" << endl;
		}

		//0100 - JUMP RELATIVE (JRP)
		if (pi.test(18)==true && pi.test(17)==false && pi.test(16)==false && pi.test(19)== false ) 
		{
			// jump relative
			ci = bitGen(adrGen(ci)+numGen(mem[adrGen(pi)])); //100 - JUMP RELATIVE (JRP)
			cout << "JUMPING. (RELATIVE)" << endl;
		}

		//0000 - JUMP ABSOLUTE (JMP)
		if (pi.test(18)==false && pi.test(17)==false && pi.test(16)==false && pi.test(19)== false ) 
		{
			//jump to value at address
			ci = mem[adrGen(pi)]; // JUMP (JMP)
		}

		//1001 - ADD
		if (pi.test(18)==false && pi.test(17)==false && pi.test(16)==true && pi.test(19)== true ) 
		{
			fAdd(mem[adrGen(pi)]);
			cout << "ADD." << endl;
		}

		//1010 - LOAD (WITHOUT NEGATING)
		if (pi.test(18)==false && pi.test(17)==true && pi.test(16)==false && pi.test(19)== true ) 
		{
			loadMem();
			cout << "LOAD. (Load without negating.)" << endl;
		}

		//1000 - JUMP TO LABEL (JLP)
		if (pi.test(18)==false && pi.test(17)==false && pi.test(16)==false && pi.test(19)== true ) 
		{
			cout << "JUMPING. (To LABEL)" << endl;
			ci = bitGen(adrGen(pi)-1);
		}

					
	}

};



//****************END OF MANCHESTER BABY SIMULATOR CLASS CODE*********************************/












//Rect struct for defining button boundaries:
struct Rect
{
	int x;
	int y;
	int width;
	int height;
};

int main()
{
	displayWelcome();
	renderSimulation();
	cout << "Closing Manchester Baby Simulator." << endl;
	return 0;
}

void clearConsole()
{
	//print endline characters to clear console. Used to improve legibility of output.

	//for reference: 54 iterations should be enough to clear a fullscreen console on a screen with a height of 1080 pixels
	for (int i=0; i<80; i++)
	{
		cout << endl;
	}
}

bool wasMousePressed()
{
	if (mouseStatePrev == 0 && mouseStateCur != 0)
	{
		return true;
	}
	return false;
}

void drawButton(Rect * btnPtr,const int x,const int y,const string text, CImg<unsigned char> & img, CImg<unsigned char> & tx_btn1,  CImg<unsigned char> & tx_btn2, const unsigned char * color){

					if (x >= btnPtr->x && x <= btnPtr->x+btnPtr->width && y >= btnPtr->y && y <= btnPtr->y+btnPtr->height)
					{
						img.draw_image(btnPtr->x,btnPtr->y,tx_btn1);
						img.draw_text(btnPtr->x+16, btnPtr->y+8, text.c_str(), color, 0, 0.7f, 26);
					}
					else
					{
						img.draw_image(btnPtr->x,btnPtr->y,tx_btn2);
						img.draw_text(btnPtr->x+16, btnPtr->y+8, text.c_str(), color, 0, 0.7f, 26);
					}
}

void renderSimulation()
{

	//initialise baby
	coreMB baby;

	//set up GUI variables:

	//load in image bitmaps:
	CImg<unsigned char> tx_LED_On("img/LEDON.bmp");
	CImg<unsigned char> tx_LED_Off("img/LEDOFF.bmp");
	CImg<unsigned char> tx_btn1("img/btndefault1.bmp");
	CImg<unsigned char> tx_btn2("img/btndefault2.bmp");
	CImg<unsigned char> tx_arrow("img/arrow2.bmp");




	//set up colours
	unsigned char white[] = { 255, 255, 255 };
	unsigned char black[] = { 0, 0, 0 };

	int storePaddingY = 160;

	//set up accumulator LEDs
	Rect accLEDs[32];

	//set up StoreLEDs
	Rect storeLEDs[32][32];

	//initialise accumulator LEDs
	for (int x = 0; x < 32; x++)
	{
			accLEDs[x].x = 100+(x*20+2);
			accLEDs[x].y = storePaddingY-80+(2);
			accLEDs[x].width = tx_LED_On.width();
			accLEDs[x].height = tx_LED_On.height();
	}



	//initialise store LEDs
	for (unsigned int y = 0; y< 32; y++)
	{
		for (unsigned int x = 0; x <32 ; x++)
		{
			storeLEDs[y][x].x = 100+(x*20+2);
			storeLEDs[y][x].y = storePaddingY+(y*20+2);
			storeLEDs[y][x].width = tx_LED_On.width();
			storeLEDs[y][x].height = tx_LED_On.height();
		}
	}

	//initialise the pointer which is to be used for drawing buttons:
	Rect *btnPtr = NULL;

	//initialise simulator buttons
	Rect btnStep = {100+30, 16, tx_btn1.width(), tx_btn1.height()};
	Rect btnRun = {200+30, 16, tx_btn1.width(), tx_btn1.height()};
	Rect btnStop = {300+30, 16, tx_btn1.width(), tx_btn1.height()};
	Rect btnLoad = {400+30, 16, tx_btn1.width(), tx_btn1.height()};
	Rect btnReset = {500+30, 16, tx_btn1.width(), tx_btn1.height()};
	Rect btnExtend = {600+30, 16, tx_btn1.width(), tx_btn1.height()};

	//set up display
	CImg<unsigned char> img(900,800, 1, 3, 0); //image for drawing current frame
	CImgDisplay dispMain(img, "Manchester Baby Simulator");


	int acccumDec = 0;
	int ciDec = 0;
	bool running=false;
	bool loaded = false;
	int scrollCounter =0;
	//GUI Loop:
	while (!dispMain.is_closed() && !dispMain.is_keyQ() && !dispMain.is_keyESC()) 
	{

		//Two main parts to this loop:
		//Process Input
		//Draw Updated GUI

		//INPUT PROCESSING:

		//process MOUSE input:
		mouseStatePrev = mouseStateCur;
		mouseStateCur = dispMain.button(); //update variables to keep track of mousebtn state

		if (baby.getStopped()){
			running = false;
		}

		if (running){
			baby.incCI();
			baby.loadIns();
			baby.decIns();
			acccumDec = baby.numGen(baby.getACC());
			ciDec = baby.numGen(baby.ci);
		}

		//check if mouse is DOWN
		if (wasMousePressed())
		{
			//check if mouse is on step button
			btnPtr = &btnStep;
			if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height && !baby.getStopped() &&loaded)
			{
				cout << "Step." << endl;
				//increment Control Instruction:
				baby.incCI();
				//fetch instruction
				baby.loadIns();
				//decode instruction (and fetch operands if necessary), then execute.
				baby.decIns();
			}


			//check if mouse is on run button
			btnPtr = &btnExtend;
			if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height )
			{


				if(dispMain.button() == 1){
					baby.modMem(1);
				}
				else if (dispMain.button() == 2){
					if (baby.mem.size() > 2) //only let the user decrease memory size down to two words.
					{
						baby.modMem(-1);
					}
				}


			}


			//check if mouse is on run button
			btnPtr = &btnRun;
			if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height && !baby.getStopped() && loaded)
			{
				//run code goes here
				cout << "Run." << endl;
				running = true;
				
			}

			//check if mouse is on stop button
			btnPtr = &btnStop;
			if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height && !baby.getStopped() && loaded)
			{
				//stop code goes here
				cout << "Stop." << endl;

				running = false;
			}

			//check if mouse is on reset button
			btnPtr = &btnReset;
			if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height && loaded)
			{
				baby.coreReset(false);
				running=false;
			}

			//check if mouse is on load button
			btnPtr = &btnLoad;
			if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height)
			{

				//load in the store from a file:
				string fileName;
				cout << "Please enter the name of the file you wish to load in the state of the memory from:" << endl;
				getline(cin, fileName);
				if (fileName!="")
				{
					ifstream file(fileName.c_str()); //open file named fileName
					if (file.is_open())
					{

						//reset baby
						baby.coreReset(true);

						//newline, just for neat formatting
						cout << endl;

						//read file line by line.
						string line = "";
						unsigned int lineCount=0;
						while (getline(file, line)) //keep getting new line from file and store it in line, until there are none left.
						{
							//if not enough memory, extend it
							if (lineCount>(baby.mem.size()-1)){
								baby.modMem(1);
							}
							//we now have a string of 1s and 0s in the string, line.
							//assign these to the bitsets in memory.

							for(int i=0; i<32; i++) //for each of the 32 bits in a row of the store
							{
								if (line.at(i)=='0')
									baby.mem.at(lineCount)[31-i]=0;
								else
									baby.mem.at(lineCount)[31-i]=1;
							}
							lineCount++; //increment lineCount to set the bits for the next row of the store.
						}

						//Done loading memory state into store.

						loaded = true;
						baby.memInitialState = baby.mem;
					}

				else
				{
					cout << "Error. Ensure that the file exists and can be opened." << endl;

				}
					
				}else{
					cout << "File name cannot be an empty string. Operation cancelled." << endl;
				}

			}

			//code to edit store data by clicking on store LEDs
			for (unsigned int y = 0; y < 32; y++)
			{
				for (int x = 0; x < 32; x++)
				{
					if (baby.mem.size() > y+scrollCounter)
					{
						//if mouse is on LED:
						if (dispMain.mouse_x() >= storeLEDs[y][x].x && dispMain.mouse_x() < storeLEDs[y][x].x+storeLEDs[y][x].width && dispMain.mouse_y() >= storeLEDs[y][x].y && dispMain.mouse_y() < storeLEDs[y][x].y+storeLEDs[y][x].height)
						{
							//flip the state of the bit we are hovering on.
							baby.mem.at(y+scrollCounter).set(31-x, !baby.mem.at(y+scrollCounter).test(31-x));
							loaded = true;
						}
					}
					
				}
			}

			acccumDec = baby.numGen(baby.getACC());
			ciDec = baby.numGen(baby.ci);

		}

		//UPDATE INSTRUCTIONS BASED ON HOVER POSITION
		const char* strIns = " ";
		
		//check if mouse is on step button
		btnPtr = &btnStep;
		if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height && !baby.getStopped())
		{
			strIns="Steps through the code, one line at a time.";
		}


		//check if mouse is on extend button
		btnPtr = &btnExtend;
		if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height )
		{
			strIns="Changes the size of available memory. Left click: increase / Right click: reduce.";
		}


		//check if mouse is on run button
		btnPtr = &btnRun;
		if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height && !baby.getStopped())
		{
			strIns="Runs the code continuously, until stopped with the 'Stop' button.";
		}

		//check if mouse is on stop button
		btnPtr = &btnStop;
		if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height && !baby.getStopped())
		{
			strIns="Stops a running simulation.";
		}

		//check if mouse is on reset button
		btnPtr = &btnReset;
		if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height)
		{
			strIns="Resets the Manchester Baby's registers and reloads the assembled program.";
		}

		//check if mouse is on load button
		btnPtr = &btnLoad;
		if (dispMain.mouse_x() >= btnPtr->x && dispMain.mouse_x() <= btnPtr->x+btnPtr->width && dispMain.mouse_y() >= btnPtr->y && dispMain.mouse_y() <= btnPtr->y+btnPtr->height)
		{
			strIns="Loads data into the store from an assembled file. (USE THE CONSOLE TO ENTER TEXT.)";
		}

		//check if mouse is on the store:
		//if mouse is on LED:
		if (dispMain.mouse_x() >= storeLEDs[0][0].x && dispMain.mouse_x() < storeLEDs[0][31].x+storeLEDs[0][31].width && dispMain.mouse_y() >= storeLEDs[0][0].y && dispMain.mouse_y() <= storeLEDs[31][0].y+storeLEDs[31][0].height)
		{
			strIns="You may click on the LEDs to edit the binary data contained by the store.";
		}


		//DRAWING GUI:

		//start by clearing screen.
		img.fill(200); //clear image (fill with a light grey) so that next frame can be drawn

		int regGap=22;

		//draw instructions:
		img.draw_text(10, 1, "Hover over a button or over the store to view instructions. Use the scroll wheel to scroll through the store.", black, 0, 0.7f, 14);

		img.draw_text(100, 60, strIns, black, 0, 0.7f, 16);


		//draw CI register
		img.draw_text(accLEDs[0].x - 93, accLEDs[0].y+regGap, "CI:", black, 0, 0.7f, 16);
				img.draw_text(accLEDs[31].x + 30, accLEDs[0].y+regGap, "(%i)", black, 0, 0.7f, 20, ciDec);
				for (int x = 0; x < 32; x++)
				{
						if (baby.ci.test(31-x))
						{
							img.draw_image(accLEDs[x].x,accLEDs[x].y+regGap,tx_LED_On);
						}
						else
						{
							img.draw_image(accLEDs[x].x,accLEDs[x].y+regGap,tx_LED_Off);
						}
				}

		//draw PI register
			img.draw_text(accLEDs[0].x - 93, accLEDs[0].y+regGap*2, "PI:", black, 0, 0.7f, 16);
			for (int x = 0; x < 32; x++)
			{
					if (baby.pi.test(31-x))
					{
						img.draw_image(accLEDs[x].x,accLEDs[x].y+regGap*2,tx_LED_On);
					}
					else
					{
						img.draw_image(accLEDs[x].x,accLEDs[x].y+regGap*2,tx_LED_Off);
					}
			}


		//draw accumulator
		img.draw_text(accLEDs[0].x - 93, accLEDs[0].y, "Accumulator:", black, 0, 0.7f, 16);
		img.draw_text(accLEDs[31].x + 30, accLEDs[0].y, "(%i)", black, 0, 0.7f, 20, acccumDec);
		for (int x = 0; x < 32; x++)
		{
				if (baby.getACC().test(31-x))
				{
					img.draw_image(accLEDs[x].x,accLEDs[x].y,tx_LED_On);
				}
				else
				{
					img.draw_image(accLEDs[x].x,accLEDs[x].y,tx_LED_Off);
				}
		}


		//update scrollCounter . Drawing of the arrow, line numbers, storeLEDs depends on scroll counter
		if (dispMain.wheel()) {
			scrollCounter -=dispMain.wheel();
			scrollCounter = (scrollCounter<0) ? 0 : scrollCounter;
			dispMain.set_wheel();
		 }

		//draw store LEDs
		for (unsigned int y = 0; y < 32; y++)
		{
			if (baby.mem.size() > y+scrollCounter)
			{
				img.draw_text(storeLEDs[0][0].x - 30, storeLEDs[y][0].y, "%i:", black, 0, 0.7f, 20, (y+scrollCounter));
			}
			for (int x = 0; x < 32; x++)
			{
				if (baby.mem.size() > y+scrollCounter)
				{
					if (baby.mem.at(y+scrollCounter).test(31-x))
					{
						img.draw_image(storeLEDs[y][x].x,storeLEDs[y][x].y,tx_LED_On);
					}
					else
					{
						img.draw_image(storeLEDs[y][x].x,storeLEDs[y][x].y,tx_LED_Off);
					}
				}
			}
		}

		//draw program counter and arrow:
		if (baby.numGen(baby.ci)+1>=scrollCounter){
		img.draw_image(100-30-tx_arrow.width(), storePaddingY+(baby.numGen(baby.ci)+1)*tx_arrow.height()-scrollCounter*20+2, tx_arrow);
		}

		drawButton(&btnStep, dispMain.mouse_x() ,dispMain.mouse_y(),"Step", img, tx_btn1,  tx_btn2, white);
		drawButton(&btnRun, dispMain.mouse_x() ,dispMain.mouse_y(),"Run", img, tx_btn1,  tx_btn2, white);
		drawButton(&btnStop, dispMain.mouse_x() ,dispMain.mouse_y(),"Stop", img, tx_btn1,  tx_btn2, white);
		drawButton(&btnReset, dispMain.mouse_x() ,dispMain.mouse_y(),"Res", img, tx_btn1,  tx_btn2, white);
		drawButton(&btnLoad, dispMain.mouse_x() ,dispMain.mouse_y(),"Load", img, tx_btn1,  tx_btn2, white);
		drawButton(&btnExtend, dispMain.mouse_x() ,dispMain.mouse_y(),"Extd", img, tx_btn1,  tx_btn2, white);

		//present drawn frame:
		dispMain.display(img).wait(10);
	}



}





void displayWelcome()
{

	clearConsole();

	//display welcome message:
	cout << endl;
	cout << "               *********************************" << endl;
	cout << "               *         COMPUTER SYSTEMS 2A:  *" << endl;
	cout << "               *           ARCHITECTURE        *" << endl;
	cout << "               * AC21009     FUNDAMENTALS      *" << endl;
	cout << "               *               AND             *" << endl;
	cout << "               *                 UNIX          *" << endl;
	cout << "               *********************************" << endl;
	cout << "               * ASSIGNMENT 3:  MANCHESTER     *" << endl;
	cout << "               *                BABY           *" << endl;
	cout << "               *                               *" << endl;
	cout << "               *     by Radu Birgauan,         *" << endl;
	cout << "               *        Can Gafuroglu,         *" << endl;
	cout << "               *        Erik Jeny,             *" << endl;
	cout << "               *        Vladislav Voicehovics  *" << endl;
	cout << "               *                               *" << endl;
	cout << "               *********************************" << endl;
	cout << endl;

}
