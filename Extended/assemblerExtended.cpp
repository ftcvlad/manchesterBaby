#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <iterator>

#include <algorithm>
#include <stdexcept>
#include <sstream>

#include <bitset>
#include <utility> //for pairs
#include <cctype>

using namespace std;

map<string, pair< vector<string> , int > > macroDefinitions;


void loadData(vector<string> & instructionLines, 	map<string,int> & symbolTable, vector<int>& lineCount);
void expandHash(vector<string> & instructionLines, 	map<string,int> & symbolTable, vector<int>& lineCount);
void loadDataFromLine(string &line, int &sourceLineCounter, int &binLineCounter, ifstream &ifs, vector<string> & instructionLines, map<string,int> & symbolTable, vector<int>& lineCount);
void loadMacros(string fileName, map<string,int> & symbolTable);

void writeData(vector<string> & instructionLines);
int operandToDec(string & goDecOperand,const std::map<string,int>& symbolTable , stringstream & mes );
void assemble( vector<string> & instructionLines,  const map<string,int> & symbolTable, vector<int>& lineCount );
string  decToBinBigEnd(int num);

//deals with ';' and ':' , fills symbol table and creates "unformatted" vector of instructions. Also deals
//with ARR declarations and # by expanding these lines to several lines which are later processed as usual.
//ignores tabs/spaces/linebreaks/comments/'label:' lines
void loadData(vector<string> & instructionLines, map<string,int> & symbolTable, vector<int>& lineCount){


		int binLineCounter=0; // to keep track of which line of binary machine code (which we will produce afterwards) we are on.
		int sourceLineCounter=0; //to keep track of which source code line we are on.

		//open file
		cout<<"Enter the name of the file to assemble: ";
		string fname;
		getline(cin,fname);
		ifstream ifs(fname.c_str());
		if (!ifs){
			throw std::invalid_argument("Couldn't open file for loading data.");
		}

		//load the macro definitions
		loadMacros(fname, symbolTable);

		cout<<'\n';
		cout << "------------------" << endl;
		cout << "Macro Definitions:" << endl;
		cout << "------------------" << endl;
		for(map<string, pair< vector <string>, int> >::const_iterator it = macroDefinitions.begin(); it != macroDefinitions.end(); ++it)
		{
			cout << "Macro Name: " << it->first << " Macro Parameters: " << it->second.second << "\n";
			for (unsigned int i=0;i<it->second.first.size();i++)
			{
				cout << "Macro Instruction Line index : " << i << " Macro Instruction Line: " << it->second.first.at(i) << "\n";
			}
		}

		cout<<'\n';



		//read data
		string line;

		cout<<"CREATING INSTRUCTION LIST ..."<<endl;
		//boolean that keeps track of whether we are dealing with a macro definition right now:
		bool isInMacro = false;
		while (getline(ifs, line,'\n')){

			//increment sourceLineCounter to update which line we are on
			sourceLineCounter++;

			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());//unix/linux have also \r

			line = line.substr(0, line.find(";"));//if has no ;, find returns npos, and all string taken

			//deal with macros:

			if (!isInMacro && line.find("%MACRO")!=string::npos){ //look for a macro in the current line if we are not already in a macro
					isInMacro=true;
			}
			else if (isInMacro && line.find("%ENDMACRO")!=string::npos){
					isInMacro=false;
					continue;
			}

			if (isInMacro){
				continue;
			}

			loadDataFromLine(line, sourceLineCounter, binLineCounter, ifs, instructionLines, symbolTable, lineCount);

		}

		ifs.close();
}

void loadMacros(string fileName, map<string,int> & symbolTable){


	    cout<<"LOADING MACROS..."<<endl;
		int sourceLineCounter = 0;

		string line;

		ifstream ifs(fileName.c_str());

		//boolean that keeps track of whether we are dealing with a macro definition right now:
		bool isInMacro = false;
		string curMacro = "";
		int curMacroParams = 0;
		while (getline(ifs, line,'\n')){

			//increment sourceLineCounter to update which line we are on
			sourceLineCounter++;

			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());//unix/linux have also \r

			line = line.substr(0, line.find(";"));//if has no ;, find returns npos, and all string taken

			//deal with macros:

			if (!isInMacro){ //look for a macro in the current line if we are not already in a macro

				if (line.find("%MACRO")!=string::npos){
					//if "%MACRO" exists in line:
					isInMacro=true;

					//tokenize line
					istringstream iss(line);
					vector<string> tokens;
					copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));

					//a correct macro definition must start with exactly 3 tokens
					if (tokens.size() != 3)
					{
						stringstream mes;
						mes << "Error at line "<<sourceLineCounter<<": A macro definition must begin with three tokens: '%MACRO', the macro name, and the number of parameters.";
						throw std::invalid_argument( mes.str().c_str() );
					}
					else if (macroDefinitions.find(tokens[1]) != macroDefinitions.end())
					{
						//if macro definition has the correct number of tokens but the name token has already been used by another macro:
						stringstream mes;
						ifs.close();
						mes << "Error at line "<<sourceLineCounter<<": A macro by the name '" << tokens[1] << "' was previously defined. You may not use the same name again as overloading macros is not supported.";
						throw std::invalid_argument( mes.str().c_str() );
					}
					else{

						//correct macro definition.
						//Start creating an entry for this macro and move on without incrementing binLineCounter.
						curMacro = tokens[1];

						bool isOnlyDigits = true; //assume true

						for (unsigned int i=0; i<tokens[2].length(); i++){
							if (isdigit(tokens[2].at(i))==false){
								isOnlyDigits = false;
								break;
							}
						}

						stringstream ss;
						ss << tokens[2];
						ss >> curMacroParams;

						if (ss.fail() || !isOnlyDigits)
						{
							ifs.close();
							stringstream mes;
							mes << "Error at line "<<sourceLineCounter<<": Invalid parameters for a macro.";
							throw std::invalid_argument( mes.str().c_str() );
						}

						//initialise macro entry by setting the number of parameters it takes.
						macroDefinitions[curMacro].second = curMacroParams;

						continue;
					}
				}
			}
			else{ //logic for if isInMacro is in this else block:

				if (line.find("%ENDMACRO")!=string::npos){

					//if "%ENDMACRO" exists in line:

					isInMacro=false;

					line = line.substr(line.find("%ENDMACRO")+9, string::npos);//remove %ENDMACRO from line
					if(line.find_first_not_of(" 	") != std::string::npos ){//space or tab
						ifs.close();
						stringstream mes;
						mes << "Error at line "<<sourceLineCounter<<": A line that ends a macro should not contain any assembler code other than '%ENDMACRO'.";
						throw std::invalid_argument( mes.str().c_str() );
					}

					cout<<"macro "<<curMacro<<" created"<<endl;
					continue;
				}
				else{	//this is NOT an %ENDMACRO line, we are still defining the macro:

					//labels in macros would be defined twice, VARs would be inserted in the middle of program. forbid!
					if (line.find(":")!=string::npos){
						stringstream mes;
						mes << "Error at line "<<sourceLineCounter<<": labels cannot be defined within macros";
						ifs.close();
						throw std::invalid_argument( mes.str().c_str() );
					}

					if (line.find('%')!=string::npos){
						string paramNoStr = line.substr(line.find('%') +1, string::npos); //get the parameter number
						int paramNo;
						stringstream ssParam;
						ssParam << paramNoStr;
						ssParam >> paramNo;
						if (paramNo > curMacroParams || !ssParam ){
							stringstream mes;
							mes << "Error at line "<<sourceLineCounter<<": Wrong parameters (not numeric or outside range)";
							ifs.close();
							throw std::invalid_argument( mes.str().c_str() );
						}
					}

					istringstream iss(line);
					vector<string> tokens;
					copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));

					if (tokens.size()==0){//lines of form "\r\n"  or  ";fsdfsdf"  or "asdf: or tabs/spaces" don't contain instructions
						continue;
					}

					macroDefinitions[curMacro].first.push_back(line);//DON'T EXPAND LINES IN loadMacro -- loadDataFromLine function is there to do it

				}
			}
		}

		ifs.close();
}

void loadDataFromLine(string &line, int &sourceLineCounter, int &binLineCounter, ifstream &ifs, vector<string> & instructionLines, map<string,int> & symbolTable, vector<int>& lineCount)
{
			//label:
			size_t ind = line.find(":");
			if (ind!=string::npos){
				//if ":" exists in line:

				//get label
				string label = line.substr(0, ind);

				//get tokens within the label substr: (expecting to find only 1)
				istringstream issL(label);
				vector<string> tokensL;
				copy(istream_iterator<string>(issL), istream_iterator<string>(), back_inserter(tokensL));

				//if there are 2 or more tokens, reject
				if (tokensL.size()!=1){
					stringstream mes;
					mes << "Error at line "<<sourceLineCounter<<": label must be a single word.";
					ifs.close();
					throw std::invalid_argument( mes.str().c_str() );
				}

				if (symbolTable.find(tokensL[0])!=symbolTable.end()){
					stringstream mes;
					mes << "Error at line "<<sourceLineCounter<<": label cannot be defined twice!";
					ifs.close();
					throw std::invalid_argument( mes.str().c_str() );
				}

				//if label contains [], reject. We don't want any mix-ups to occur with arrays.
				if (label.find("]")!=string::npos || label.find("[")!=string::npos ){
					stringstream mes;
					mes << "Error at line "<<sourceLineCounter<<": label must not contain ']' or '['";
					ifs.close();
					throw std::invalid_argument( mes.str().c_str() );
				}

				//remove label
				line = line.substr(ind+1, line.length());

				//
				if (line.length()!=0){//'label:;zzz' case
					symbolTable[label]=binLineCounter;
				}
			}



			//check for immediate addressing and arrays here, as need to insert>1 line which would screw labels
			//if done afterwards. With arrays cannot push these lines to the end. Error checking is as usual afterwards

			istringstream iss(line);
			vector<string> tokens;
			copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));

			if (tokens.size()!=0){//skipped lines of form "\r\n"  or  ";fsdfsdf"  or "asdf: or tabs/spaces" don't contain instructions

				//check if first token is a macro:
				if (macroDefinitions.find(tokens[0]) != macroDefinitions.end()){

					//this line is a macro call

					//number of tokens should be macro parameters + 1
					if (tokens.size() == macroDefinitions[tokens[0]].second + 1){

						//correct call to macro
						//push macro's instruction lines to the vector of instruction lines:

						for (unsigned int i=0; i<macroDefinitions[tokens[0]].first.size(); i++){

							string macroLine = macroDefinitions[tokens[0]].first.at(i);

							//replace parameter placeholder
							if (macroLine.find('%') != string::npos){
								//we found a percentage sign - insert parameter here.
								string paramNoStr = macroLine.substr(macroLine.find('%') +1, string::npos); //get the parameter number
								int paramNo;
								stringstream ssParam;
								ssParam << paramNoStr;
								ssParam >> paramNo;

								macroLine.replace(macroLine.find('%'), paramNoStr.size()+1, tokens[paramNo]);
							}

							//now that we ensured the right information was put into the instruction line, we can put it through this method recursively to push it into the instructions list:

							loadDataFromLine(macroLine, sourceLineCounter, binLineCounter, ifs, instructionLines, symbolTable, lineCount);
						}

					}
					else{

						//incorrect call to macro
						stringstream mes;
						mes << "Error at line "<<sourceLineCounter<<": Macro '" << tokens[0] << "' must be given exactly " << macroDefinitions[tokens[0]].second << " parameter(s).";
						ifs.close();
						throw std::invalid_argument( mes.str().c_str() );
					}

				}
				else{

					//not a macro, handle as usual.

					 if (tokens[0]=="ARR"){//expand ARR to number of VARS.
						//ARR label (if any) points to the length of array. label[0] points to 1st element in the array

						int len = tokens.size()-1;
						stringstream crLen;
						crLen<<"VAR "<< len;
						instructionLines.push_back(crLen.str());//binLineCounter incremented in the end once as well
						lineCount.push_back(sourceLineCounter);
						binLineCounter+=1;

						for (unsigned int i=1;i<tokens.size();i++){
							stringstream ss;
							ss<<"VAR "<< tokens[i];
							string arrValLine = ss.str();
							instructionLines.push_back(arrValLine);
							lineCount.push_back(sourceLineCounter);
							binLineCounter++;
						}


					}
					else{//ordinary normal great simple line everyone loves AND lines including #
						instructionLines.push_back(line);
						lineCount.push_back(sourceLineCounter);
						binLineCounter+=1;
					}


				}//End of if first token is not a macro

			} //end of if token count is NOT zero
}


void expandHash(vector<string> & instructionLines, 	map<string,int> & symbolTable, vector<int>& lineCount){

	cout<<"EXPANDING IMMEDIATE ADDRESSING LINES (# lines)..."<<endl;
	for (unsigned int i=0; i<instructionLines.size();i++ ){

			istringstream iss(instructionLines[i]);
			vector<string> tokens;
			copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));


			if (tokens.size()==2 && tokens[1][0]=='#'){//# is possible only in 2 token commands, as the first character of the second token.

				std::map<string,int>::const_iterator it = symbolTable.find(tokens[1]);
				if (it!=symbolTable.end()){
					stringstream ss;
					ss<<tokens[0]<<" "<< it->second;
					string line = ss.str();
					instructionLines[i] = line;//just replace LDN #5 with LDN x
				}
				else{
					if (tokens[0]=="VAR"){
						//in this case, just drop #
						stringstream ss;
						ss<<"VAR "<<tokens[1].substr(1,string::npos);
						string line1 = ss.str();
						instructionLines[i] = line1;

						symbolTable[tokens[1]]=i;
					}
					else{
						//in this case, we need to expand to 2 lines. ldn #5 ->ldn instrLines.size() , VAR 5

						//Instr line
						int size = instructionLines.size();//VAR inserted there
						stringstream ss;
						ss<<tokens[0]<<" "<<size;
						string line1 = ss.str();

						instructionLines[i] = line1;
						symbolTable[tokens[1]]=size;

						//VAR line
						stringstream ss3;
						ss3<<"VAR "<<tokens[1].substr(1,string::npos);
						string line3 = ss3.str();
						instructionLines.push_back(line3);
						lineCount.push_back(lineCount[i]);//here error will point to macro. if VAR
					}
				}
			}
	}

	cout<<'\n';
	cout << "INSTRUCTION LINES & LINE COUNT:" << endl;
	for(unsigned int i = 0; i < instructionLines.size(); i++)
	{
		cout << "index: " << i << " instruction: " << instructionLines[i] << " count: " <<  lineCount[i] << "\n";
	}

	cout<<'\n';
	cout << "-------------" << endl;
	cout << "SYMBOL TABLE:" << endl;
	cout << "-------------" << endl;
	for(map<string, int>::const_iterator it = symbolTable.begin(); it != symbolTable.end(); ++it)
	{
		std::cout << it->first << ": " << it->second << "\n";
	}


}

//instructions here have 1 or 2 tokens ( thus myarr[x] cannot have spaces ).
//processes  instructions obtained in 1st traversal to get binary
void assemble( vector<string> & instructionLines,  const map<string,int> & symbolTable, vector<int>&lineCount ){

	cout<<'\n';
	cout<<"ASSEMBLING BINARY LINES..."<<endl;
	//for each element in the instructionLines vector:
	for (unsigned int i=0; i<instructionLines.size();i++ ){

		//get tokens:
		istringstream iss(instructionLines[i]);
		vector<string> tokens;
		copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));

		//prepare error message:
		stringstream mes;
		mes << "Error at line "<<lineCount[i]<<": '" << instructionLines[i] << "'. ";


		if (tokens.size()>2 || tokens.size()==0){
			mes<<"Wrong number of arguments.";
			 throw std::invalid_argument( mes.str().c_str() );
		}
		else if (tokens.size()==1){//instructions that don't require operands
			if (tokens[0]=="STP"){
				instructionLines[i] = "00000000000001110000000000000000";
			}
			else if (tokens[0]=="CMP"){
				instructionLines[i] = "00000000000000110000000000000000";
			}
			else{
				mes<<"Instruction doesn't exist.";
				 throw std::invalid_argument( mes.str().c_str() );
			}
		}
		else{//instructions that require 1  operand

			int decOperand;
			string indAddr="";

			//check for use of array
			if (tokens[1].find("[")!=string::npos && tokens[1].find("]")!=string::npos){

				//get value from array
				int startPos = tokens[1].find("[");
				int endPos = tokens[1].find("]");
				int indLen = endPos-startPos-1;

				//check errors, find label
				string arrName = tokens[1].substr(0,startPos);
				std::map<string,int>::const_iterator 	labIt = symbolTable.find(arrName);
				if ( labIt==symbolTable.end() || (indLen<1)){
					mes<<"Wrong array name or operator[].";
					throw std::invalid_argument( mes.str().c_str() );
				}

				//find index
				string index = tokens[1].substr(startPos+1,indLen);
				int decIndex = operandToDec(index,symbolTable, mes);


				if (symbolTable.find(index)!=symbolTable.end()){//if index is a variable
					stringstream sst;
					sst<<"1"<< decToBinBigEnd(decIndex);
					 indAddr = sst.str();
					 decOperand = labIt->second;
				}
				else{
					decOperand = labIt->second +1 + decIndex;//+1 as labIt points to array length
				}

			}
			else{
				decOperand = operandToDec(tokens[1],symbolTable, mes);
			}


			// + and - ints are already represented correctly (- as two's complement),so, just get and reverse
			bitset<32> x(decOperand);
			string buildMe  = x.to_string();
			std::reverse(buildMe.begin(), buildMe.end());

			if (tokens[0]!="VAR"){//if it is == VAR don't have to do anything

				if ( decOperand<0){//line addresses are 0-31 EXTEND MEMORY?
					mes<<"Operand values can only be only larger than 0";
					 throw std::invalid_argument( mes.str().c_str() );
				}

				string toInsert;
				if (tokens[0]=="LDN"){
					toInsert = "0010";
				}
				else if (tokens[0]=="JMP"){
					toInsert = "0000";
				}
				else if (tokens[0]=="JRP"){
					toInsert = "0100";
				}
				else if (tokens[0]=="STO"){
					toInsert = "0110";
				}
				else if (tokens[0]=="SUB"){
					toInsert = "0001";
				}
				else if (tokens[0]=="ADD"){
					toInsert = "1001";
				}
				else if (tokens[0]=="LOAD"){
					toInsert = "1010";
				}
				else if (tokens[0]=="JLP"){
					toInsert = "1000";
				}
				else{
					mes<<"Instruction doesn't exist.";
					throw std::invalid_argument( mes.str().c_str() );
				}

				buildMe.replace(12,4,toInsert);
				buildMe.replace(16,indAddr.length(),indAddr);//this can insert nothing OR address from which to take index (16th bit indicates this)

			}
			instructionLines[i] = buildMe;//could write to file right away, but want to ensure that no mistakes first
		}
	}

}


string  decToBinBigEnd(int num){
	ostringstream ss;
	ss<< num%2;
	while(num/2>0){
		num=num/2;
		ss<< num%2;
	}

	return ss.str();
}


//searches for operand in symbolTable (in case it's a label) and if not found, tries to read it as a value
int operandToDec(string & goDecOperand,const std::map<string,int>& symbolTable , stringstream & mes ){
				int result;
				std::map<string,int>::const_iterator it = symbolTable.find(goDecOperand);

				if (it == symbolTable.end()){
						//not found in symbol table

						//try to treat as a value, throw exception if invalid
						char c;
						stringstream ss(goDecOperand);
						ss >> result;
						if (ss.fail() || ss.get(c)) {//if 'not integer' or 'integer > 2^31-1'(as 1 bit reserved for sign) or '12345asdf'
							mes<<"Invalid operand.";
							throw std::invalid_argument( mes.str().c_str() );
						}
				}
				else{
					result=it->second;
				}

				return result;
}


void writeData(vector<string> & instructionLines){

	cout<<"WRITING DATA..."<<endl;
	ofstream ofs;

	string fname;
	cout<<"Enter name of the file to save to: ";
	getline(cin,fname);
	ofs.open(fname.c_str(), ios_base::out);

	for (unsigned int i=0; i<instructionLines.size();i++){
		ofs<<instructionLines[i]<<'\n';
	}

	ofs.close();
	if ( ! ofs ) {
	  cout<<"an error occured while writing to file. "<<endl;
	  return;
	}

	cout<<"FINISHED."<<endl;

}


int main(){

	vector<string> instructionLines(0);
	vector<int> lineCount(0);//just for error messages
	map<string,int> symbolTable;

	  try{
		loadData(instructionLines, symbolTable, lineCount);
		expandHash(instructionLines, symbolTable, lineCount);





		assemble(instructionLines, symbolTable, lineCount);

		writeData(instructionLines);


		for (unsigned int i=0;i<instructionLines.size();i++){
				cout<<i<<": "<<instructionLines[i]<<endl;
			}
	  }
	  catch (exception& e){
	    cout << e.what() << '\n';
	  }


	return 0;
}
