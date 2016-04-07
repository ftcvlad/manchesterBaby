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

using namespace std;

void assemble( vector<string> & instructionLines,  const map<string,int> & symbolTable, vector<int>& lineCount );
void loadData(vector<string> & instructionLines, 	map<string,int> & symbolTable, vector<int>& lineCount);
void writeData(vector<string> & instructionLines);

//deals with ';' and ':' , fills symbol table and creates "unformatted" vector of instructions
void loadData(vector<string> & instructionLines, 	map<string,int> & symbolTable, vector<int>& lineCount){

		int acLineCounter=0;
		int lineCounter=0;

		//open file
		cout<<"Enter the name of the file to assemble: ";
		string fname;
		getline(cin,fname);
		ifstream ifs(fname.c_str());
		if (!ifs){
			throw std::invalid_argument("Couldn't open file for loading data. Ensure file exists and can be opened.");
		}
		cout << "Assembly code will be scanned for symbols." << endl;

		//read data
		string line;
		while (getline(ifs, line,'\n')){

			lineCounter++;

			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());//unix/linux have anlso \r

			line = line.substr(0, line.find(";"));//if has no ;, find returns npos, and all string taken

			int ind = line.find(":");
			if (ind!=string::npos){
				string label = line.substr(0, ind);


				istringstream iss(label);
				vector<string> tokens;
				copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));
				if (tokens.size()!=1){
					stringstream mes;
					 mes << "Error line "<<lineCounter<<": label must be a single word";
					 ifs.close();
					 throw std::invalid_argument( mes.str().c_str() );
				}

				line = line.substr(ind+1, line.length());

				if (line.length()!=0){//'label:;zzz' case
					symbolTable[label]=acLineCounter;
				}



			}

			if (line.length()==0){//lines of form "\r\n"  or  ";fsdfsdf"  or "asdf:" don't contain instructions
				continue;
			}

			instructionLines.push_back(line);
			lineCount.push_back(lineCounter);
			acLineCounter++;

		}

		ifs.close();
}




void assemble( vector<string> & instructionLines,  const map<string,int> & symbolTable, vector<int>&lineCount ){

	for (unsigned int i=0; i<instructionLines.size();i++ ){

		cout<<instructionLines[i]<<endl;
	}

	for (unsigned int i=0; i<instructionLines.size();i++ ){
		istringstream iss(instructionLines[i]);
		vector<string> tokens;
		copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter(tokens));

		stringstream mes;
		mes << "Error line "<<lineCount[i]<<": '" << instructionLines[i] << "'. ";
		std::map<string,int>::const_iterator it;

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

			//obtain numeric value of 2nd token
			it = symbolTable.find(tokens[1]);
			int dec;
			if (it == symbolTable.end()){//not found in symbol table
				char c;
				stringstream ss(tokens[1]);
				ss >> dec;
				if (ss.fail() || ss.get(c)) {//if 'not integer' or 'integer > 2^31-1'(as 1 bit reserved for sign) or '12345asdf'
					mes<<"Invalid operand.";
					throw std::invalid_argument( mes.str().c_str() );

				}
			}
			else{
				dec=it->second;
			}

			// + and - ints are already represented correctly (- as two's complement),so, just get and reverse
			std::bitset<32> x(dec);
			string buildMe  = x.to_string();
			std::reverse(buildMe.begin(), buildMe.end());

			if (tokens[0]!="VAR"){//if it is == VAR don't have to do anything

				if (dec>31 || dec<0){//line addresses are 0-31
					mes<<"Operand values can be 0-31";
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
				else{
					mes<<"Instruction doesn't exist.";
					throw std::invalid_argument( mes.str().c_str() );
				}

				buildMe.replace(12,4,toInsert);
			}
			instructionLines[i] = buildMe;//could write to file right away, but want to ensure that no mistakes first
		}
	}
}


void writeData(vector<string> & instructionLines){

	ofstream ofs;

	string fname;
	cout<<"Enter the name of the file to save to: ";
	getline(cin,fname);
	ofs.open(fname.c_str(), ios_base::out);

	for (unsigned int i=0; i<instructionLines.size();i++){
		ofs<<instructionLines[i]<<'\n';
	}

	ofs.close();
	if ( ! ofs ) {
	  cout<<"An error occured while writing to file. "<<endl;
	  return;
	}

}


int main(){

	vector<string> instructionLines(0);
	vector<int> lineCount(0);//just for error messages
	map<string,int> symbolTable;



	  try{
		loadData(instructionLines, symbolTable, lineCount);
		cout << "Done scanning for symbols. The symbols found are listed below:" << endl;

		cout << "-------------" << endl;
		cout << "SYMBOL TABLE:" << endl;
		cout << "-------------" << endl;
		for(map<string, int>::const_iterator it = symbolTable.begin(); it != symbolTable.end(); ++it)
		{
		    std::cout << it->first << ": " << it->second << "\n";
		}

		cout << "Instructions will now be isolated:" << endl;
		
		assemble(instructionLines, symbolTable, lineCount);

		cout << "Machine code assembled from instructions:" << endl;
		for (unsigned int i=0;i<instructionLines.size();i++){
				cout<<instructionLines[i]<<endl;
		}

		cout << "Machine code will now be written to file. " << endl;
		writeData(instructionLines);

		
		cout << "Assembly complete." << endl;
	  }
	  catch (exception& e){
	    cout << e.what() << '\n';
	  }






	return 0;
}
