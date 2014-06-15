#include <import.hpp>

namespace cses {


Import::Import() {
}

Import::~Import() {
}
	
void Import::process(std::istream &zipData) {
	string fileName = getFileStoragePath(saveStreamToFile(zipData));
	
	char tempName[] = "zipXXXXXX";
	mkdtemp(tempName);
	string dirName = tempName;				
	
	string command = "unzip " + fileName + " -d " + dirName;
	system(command.c_str());
	DIR *dir = opendir((dirName + "/.").c_str());
	struct dirent *d;
	map<string, pair<vector<string>, vector<string>>> data;
	while ((d = readdir(dir)) != NULL) {
		string taskName = d->d_name;
		if (taskName == ".") continue;
		if (taskName == "..") continue;
		DIR *dir2 = opendir((dirName + "/" + taskName + "/.").c_str());
		struct dirent *d2;
		vector<string> inputData, outputData;
		while ((d2 = readdir(dir2)) != NULL) {
			string fileName = d2->d_name;
			if (fileName == ".") continue;
			if (fileName == "..") continue;
			if (fileName == "task.nfo") continue;
			if (fileName.find(".in") != string::npos) inputData.push_back(fileName);
			if (fileName.find(".IN") != string::npos) inputData.push_back(fileName);
			if (fileName.find(".out") != string::npos) outputData.push_back(fileName);
			if (fileName.find(".OUT") != string::npos) outputData.push_back(fileName);
		}
		sort(inputData.begin(), inputData.end());
		sort(outputData.begin(), outputData.end());
		while (outputData.size() < inputData.size()) outputData.push_back("");
		data[taskName] = make_pair(inputData, outputData);
	}	
	
	for (auto x : data) {
		string taskName = x.first;
		tasks.push_back(taskName);
		for (size_t i = 0; i < data[x.first].first.size(); i++) {
			string newInputName = data[x.first].first[i];
			string newOutputName = data[x.first].second[i];		
			FileSave inputSaver, outputSaver;
			inputSaver.writeFileContents(dirName + "/" + x.first + "/" + newInputName);
			if (newOutputName != "") {
				outputSaver.writeFileContents(dirName + "/" + x.first + "/" + newOutputName);
			}
			string inputHash = inputSaver.save();
			string outputHash = outputSaver.save();
			inputs[taskName].push_back(make_pair(inputHash, newInputName));
			outputs[taskName].push_back(make_pair(outputHash, newOutputName));
		}
		std::ifstream info;
		info.open(dirName + "/" + x.first + "/task.nfo");
		if (info.is_open()) {
			while (true) {
				string header;
				info >> header;
				if (!info.good() || header != "group") break;
				int points;
				info >> points;
				vector<string> list;
				while (true) {
					string prefix;
					info >> prefix;
					if (prefix == "end") break;
					list.push_back(prefix);
				}
				groups[taskName].push_back(make_pair(points, list));
			}
		}
	}
	
	
	command = "rm -rf " + dirName;
	system(command.c_str());	
}

}



