#include <import.hpp>

namespace cses {


Import::Import() {
}

Import::~Import() {
}
	
void Import::process(std::istream &zipData) {
}

vector<string> Import::tasks() {
	vector<string> t;
	return t;
}
	
// map<string,vector<pair<string,string>>> Import::inputs {
// 	map<string,vector<pair<string,string>>> m;
// 	return m;
// }
// 
// map<string,vector<pair<string,string>>> Import::outputs {
// 	map<string,vector<pair<string,string>>> m;
// 	return m;
// }
	
}

// 				char directoryName[] = "zipXXXXXX";
// 				mkdtemp(directoryName);
// 				string newName = directoryName;				
// 				c.form.package.value()->save_to(newName + "/pelle.zip");
// 				string command = "unzip " + newName + "/pelle.zip -d " + newName;
// 				system(command.c_str());
// 				DIR *dir = opendir((newName + "/.").c_str());
// 				struct dirent *d;
// 				map<string, pair<vector<string>, vector<string>>> data;
// 				while ((d = readdir(dir)) != NULL) {
// 					string dirName = d->d_name;
// 					if (dirName == ".") continue;
// 					if (dirName == "..") continue;
// 					if (dirName == "pelle.zip") continue;
// 					DIR *dir2 = opendir((newName + "/" + dirName + "/.").c_str());
// 					struct dirent *d2;
// 					vector<string> inputs, outputs;
// 					while ((d2 = readdir(dir2)) != NULL) {
// 						string fileName = d2->d_name;
// 						if (fileName == ".") continue;
// 						if (fileName == "..") continue;
// 						if (fileName.find(".in") != string::npos) inputs.push_back(fileName);
// 						if (fileName.find(".IN") != string::npos) inputs.push_back(fileName);
// 						if (fileName.find(".out") != string::npos) outputs.push_back(fileName);
// 						if (fileName.find(".OUT") != string::npos) outputs.push_back(fileName);
// 					}
// 					sort(inputs.begin(), inputs.end());
// 					sort(outputs.begin(), outputs.end());
// 					while (outputs.size() < inputs.size()) outputs.push_back("");
// 					data[dirName] = make_pair(inputs, outputs);
// 				}
// 				BOOSTER_INFO("lol") << "apina";


// 					for (size_t i = 0; i < data[x.first].first.size(); i++) {
// 						BOOSTER_INFO("lol") << "cembalo";
// 						File newInput;
// 						File newOutput;
// 						string newInputName = data[x.first].first[i];
// 						string newOutputName = data[x.first].second[i];
// 						FileSave inputSaver, outputSaver;
// 						
// 						char *buffer;
// 						FILE *file;
// 						int fsize;
// 						
// 						file = fopen((newName + "/" + x.first + "/" + newInputName).c_str(), "rb");
// 						fseek(file, 0, SEEK_END);
// 						fsize = ftell(file);
// 						rewind(file);
// 						buffer = (char*)malloc(sizeof(char)*fsize);
// 						fread(buffer, 1, fsize, file);
// 						fclose(file);
// 						//inputSaver.write(buffer, fsize);
// 						free(buffer);
// 
// 						file = fopen((newName + "/" + x.first + "/" + newOutputName).c_str(), "rb");
// 						fseek(file, 0, SEEK_END);
// 						fsize = ftell(file);
// 						rewind(file);
// 						buffer = (char*)malloc(sizeof(char)*fsize);
// 						fread(buffer, 1, fsize, file);
// 						fclose(file);
// 						//outputSaver.write(buffer, fsize);
// 						free(buffer);
// 
// 						string inputHash = inputSaver.save();
// 						string outputHash = outputSaver.save();
// 						
// 						newInput.hash = inputHash;
// 						newOutput.hash = outputHash;
// 						shared_ptr<TestCase> newCase(new TestCase());
// 						newCase->input = newInput;
// 						newCase->output = newOutput;
// 						newCase->group = group;
//  						db->persist(newCase);
//  						group->tests.push_back(move(newCase));
// 					}


// 				command = "rm -rf " + newName;
// 				system(command.c_str());

