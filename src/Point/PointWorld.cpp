/*
 * PointWorld.cpp
 *
 *  Created on: Nov 2, 2010
 *      Author: anthony
 */

#include "PointRep.h"
#include <sstream>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
//#include <boost/tokenizer.hpp>
//#include <boost/token_functions.hpp>
#include "CTokenizer.h"

namespace pointrep {
using namespace std;
using namespace adn;

//boost::char_separator<char> sep("\t ");
//typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

PointWorld::PointWorld() : World() {
	_binaryClass = false;
	_binClasses = pair<Associating*, Associating*>(NULL,NULL);

	_nClasses = 0;
	_trainIdx = 0;
	_testIdx = 0;
	_predIdx = 0;

	_regInt(_normClasses, "normaliseClasses");
	_regInt(_normData, "normaliseData");
	_regIntOp(_smallClassMultiplier, "smallClassMult", 0);
	_regIntOp(_epochSampleSize, "epochSampleSize", 0);
	//_regIntOp(_normGlobal, "normaliseGlobal", 0);
	_maxClassSize = 0;
}

PointWorld::~PointWorld() {

}


void PointWorld::createInstance(InstInfo& t, int set) {
	World::createInstance(t, set);

	PtInstInfo& bt = static_cast<PtInstInfo&>(t);

	if (set == trainSet) {
		// find instance from set
		int thisClass;

		if (_trainIdx >= _trainingOrder.size()) {
			logout << "requesting training sample after end of set";
			writeBufWarn();
			_trainIdx = 0;
		}
		int sampleIdx = _trainingOrder[_trainIdx++];

		// load data
		bt._features = _trainingFt[sampleIdx];
		bt._currentClass = _trainingClasses[sampleIdx];

		// replace any missing values with random
		const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
		const vector<int>& fClasses = PointRep::_brInstance->_featureClasses;
		vector<FValue>::iterator iter;
		for (unsigned int i=0; i<fTypes.size(); ++i) {
			if (fTypes[i] != iClass) continue;
			if (bt._features[i].iVal == MISSINGCLASS) bt._features[i].iVal = _RandOf(fClasses[i]);
		}
		// don't need to count instances, epoch size def'd elsewhere and will move to test mode
	} else if (set == testSet) {
		// test mode, incremental selection
		if (_testIdx >= _testFt.size()) {
			logout << "request for test after finished";
			writeBufWarn();
			_testIdx = 0;
		}

		// create instance, load data
		const Associating* thisClass = _testClasses[_testIdx];
		bt._features = _testFt[_testIdx];

		// replace any missing values with random
		const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
		const vector<int>& fClasses = PointRep::_brInstance->_featureClasses;
		vector<FValue>::iterator iter;
		for (unsigned int i=0; i<fTypes.size(); ++i) {
			if (fTypes[i] != iClass) continue;
			if (bt._features[i].iVal == MISSINGCLASS) bt._features[i].iVal = _RandOf(fClasses[i]);
		}

		bt._currentClass = thisClass;
		++_testIdx;
	} else if (set == predSet) {
		// test mode, incremental selection
		if (_predIdx >= _predFt.size()) {
			logout << "request for prediction entry after finished";
			writeBufWarn();
			_predIdx = 0;
		}

		// create instance, load data
		bt._features = _predFt[_predIdx];

		// replace any missing values with random
		const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
		const vector<int>& fClasses = PointRep::_brInstance->_featureClasses;
		vector<FValue>::iterator iter;
		for (unsigned int i=0; i<fTypes.size(); ++i) {
			if (fTypes[i] != iClass) continue;
			if (bt._features[i].iVal == MISSINGCLASS) bt._features[i].iVal = _RandOf(fClasses[i]);
		}

		bt._currentClass = NULL;
		++_predIdx;
	}
}

int PointWorld::remainingInsts(int set) const {
	if (set == trainSet)
		return _trainingOrder.size() - _trainIdx;
	else if (set == testSet)
		return _testFt.size() - _testIdx;
	else if (set == predSet)
		return _predFt.size() - _predIdx;
}

void PointWorld::start(int set)
{
	if (set == trainSet) {
		// create training set
		_trainingOrder.clear();
		// choose class randomly
		if (_smallClassMultiplier) {
			for (int c=0; c<_nClasses; ++c) {
				// choose random starting index, so don't always repeat
				// the first elements
				int idx = _RandOf(_classMap[c].size());
				int nSamples;
				if (_classMap[c].size() == _maxClassSize) nSamples = _maxClassSize;
				else nSamples = _classMap[c].size() * _smallClassMultiplier;

				for (int sample = 0; sample < nSamples; sample++) {
					if (idx >= _classMap[c].size()) idx = 0;
					_trainingOrder.push_back(_classMap[c][idx++]);
				}
			}
		} else if (_normClasses) {
			for (int c=0; c<_nClasses; ++c) {
				// choose random starting index, so don't always repeat
				// the first elements
				int idx = _RandOf(_classMap[c].size());
				for (int sample = 0; sample < _maxClassSize; sample++) {
					if (idx >= _classMap[c].size()) idx = 0;
					_trainingOrder.push_back(_classMap[c][idx++]);
				}
			}
		} else {
			_trainingOrder.resize(_trainingFt.size());
			for (unsigned int i=0; i<_trainingFt.size(); ++i) _trainingOrder[i] = i;
		}

		// shuffle order vector
		_trainIdx = 0;
		random_shuffle(_trainingOrder.begin(), _trainingOrder.end());

		// limit size if specified
		if (_epochSampleSize) {
			if (_epochSampleSize < _trainingOrder.size()) {
				_trainingOrder.resize(_epochSampleSize);
			} else {
				// repeat until appropriate size
				int newSamples = _epochSampleSize - _trainingOrder.size();
				for (int s=0; s<newSamples; ++s) {
					_trainingOrder.push_back(_trainingOrder[s]);
				}
			}
		}
	} else if (set == testSet) {
		_testIdx = 0;
	} else if (set == predSet) {
		_predIdx = 0;
	}
}

const string& PointWorld::getStateString(InstInfo& t) const {
	PtInstInfo& bt = static_cast<PtInstInfo&>(t);

	ostringstream oss;
	vector<FValue>::iterator iter;
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	oss << "[";
	for (unsigned int i=0; i<bt._features.size(); ++i) {
		if (i != 0) oss << ",";
		if (fTypes[i] == iClass || fTypes[i] == iSeq)
			oss << bt._features[i].iVal;
		else if (fTypes[i] == fSeq)
			oss << bt._features[i].fVal;
	}
	oss << "]";
	_displayCache = oss.str();
	return _displayCache;
}

Associating* PointWorld::getClassObj(int num) const
{
	// assume we can make an int out of it
	//int num = bin2Int(val, 0, val.size());

	// create class obj if not found
	tr1::unordered_map<int, PointClass*>::iterator fIter
		= _classObjs.find(num);
	if (fIter == _classObjs.end()) {
		PointClass* newClass = new PointClass(num);
		_allClasses.push_back(newClass);
		_classObjs[num] = newClass;
		return newClass;
	} else {
		return fIter->second;
	}
}

pair<Associating*, Associating*> PointWorld::getBinaryClasses() const
{
	// returns the two classes used for off and on values
	// assume we are using a binary output at the moment

	// assume classes don't change, check if have been defined
	if (! _binClasses.first) {
		_binClasses.first = getClassObj(0);
		_binClasses.second = getClassObj(1);
	}

	return _binClasses;
}


bool PointWorld::loadARFF(string filename, int set) // always sets feature types (could make flag to choose)
{
	// attempt to read file
	vector<vector<FValue> > newFeatures;
	vector<int> newClasses;

	// read from file, and set feature types
	//vector<int> featureTypes;
	if (! readARFF(filename, newFeatures, newClasses, true /*(set != predSet)*/)) return false;

	return addSet(newFeatures, newClasses, set);
}

bool PointWorld::loadCSV(string filename, int set) // always sets feature types (could make flag to choose)
{
	// attempt to read file
	vector<vector<FValue> > newFeatures;
	vector<int> newClasses;

	// read from file, and set feature types
	if (! readCSV(filename, newFeatures, newClasses)) return false;

	return addSet(newFeatures, newClasses, set);
}

bool PointWorld::loadMNIST(string imgfile, string labelfile, int set)
{
	// attempt to read file
	vector<vector<FValue> > newFeatures;
	vector<int> newClasses;

	// read from file, and set feature types
	//vector<int> featureTypes;
	if (! readLabels(labelfile, newClasses)) return false;
	if (! readImages(imgfile, newFeatures)) return false;

	vector<int>& featureTypes = PointRep::_brInstance->_featureTypes;
    vector<int>& featureClasses = PointRep::_brInstance->_featureClasses;
    featureTypes.assign(newFeatures[0].size(), iSeq);
    featureClasses.assign(newFeatures[0].size(), 0);

    double fixedMean = 255.0 / 2.0;
	return addSet(newFeatures, newClasses, set, true, &fixedMean);
}

#define NORMGLOBAL 2
bool PointWorld::addSet(vector<vector<FValue> >& newFeatures, vector<int>& newClasses, int set, bool useGlobal, double* givenMean) {

	vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	vector<pair<FValue,FValue> >& fRanges = PointRep::_brInstance->_featureRanges;

	if (_normData == NORMGLOBAL) useGlobal = true;

	// examine to extend range
	bool first = _trainingFt.empty() && _testFt.empty();
	if (fRanges.size() < fTypes.size()) fRanges.resize(fTypes.size());

	// find means & ranges
	vector<vector<FValue> >::iterator iter;
	//vector<double> means(fTypes.size(), 0.0);
	bool setMeanSD = _means.empty();
	if (setMeanSD) {
		_means.resize(fTypes.size(), 0.0);
		_globmean = 0.0;
	}

	double counts = 0.0, globcounts = 0.0;
	_foreach(newFeatures, iter) {
		counts += 1.0;
		const vector<FValue>& features = *iter;
		for (unsigned int idx=0; idx < features.size(); ++idx) {
			// if sets were empty, initialise range for each feature to values in first row
			int fType = fTypes[idx];
			if (first) {
				double val;
				val = (fType == iSeq ? features[idx].iVal : features[idx].fVal);
				if (setMeanSD) {
					_means[idx] = val;
					globcounts += 1.0;
					_globmean += (val - _globmean) / globcounts;
				}
				fRanges[idx].first = fRanges[idx].second = features[idx];
				continue;
			}

			// update range
			else if (fType == iSeq) {
				int thisVal = features[idx].iVal;
				if (thisVal < fRanges[idx].first.iVal)
					fRanges[idx].first.iVal = thisVal;
				if (thisVal > fRanges[idx].second.iVal)
					fRanges[idx].second.iVal = thisVal;
				if (setMeanSD) {
					_means[idx] += (thisVal - _means[idx]) / counts;
					globcounts += 1.0;
					_globmean += (thisVal - _globmean) / globcounts;
				}
			} else if (fType == fSeq) {
				float thisVal = features[idx].fVal;
				if (thisVal < fRanges[idx].first.fVal)
					fRanges[idx].first.fVal = thisVal;
				if (thisVal > fRanges[idx].second.fVal)
					fRanges[idx].second.fVal = thisVal;
				if (setMeanSD) {
					_means[idx] += (thisVal - _means[idx]) / counts;
					globcounts += 1.0;
					_globmean += (thisVal - _globmean) / globcounts;
				}
			}
		}
		if (first) first = false;
	}

	if (useGlobal && givenMean && setMeanSD) _globmean = *givenMean;

	// do normalising
	if (_normData) {
		// find stddevs
		if (setMeanSD) {
			vector<double> ssavs(fTypes.size(), 0.0);
			double ssav = 0.0;
			counts = 0.0;
			globcounts = 0.0;
			_foreach(newFeatures, iter) {
				counts += 1.0;
				const vector<FValue>& features = *iter;
				for (unsigned int idx=0; idx < features.size(); ++idx) {
					globcounts += 1.0;

					int fType = fTypes[idx];
					double thisVal;
					if (fType == iSeq) thisVal = features[idx].iVal;
					else if (fType == fSeq) thisVal = features[idx].fVal;
					else continue;

					if (useGlobal) {
						double sq = (thisVal - _globmean) * (thisVal - _globmean);
						ssav += (sq - ssav) / globcounts;
					} else {
						double sq = (thisVal - _means[idx]) * (thisVal - _means[idx]);
						ssavs[idx] += (sq - ssavs[idx]) / counts;
					}
				}
			}
			if (useGlobal) _globsd = sqrt(ssav);
			else {
				_sds.resize(fTypes.size(), 0.0);

				for (unsigned int idx=0; idx < fTypes.size(); ++idx) {
					int fType = fTypes[idx];
					if (fType == iClass) continue; // ignore range
					_sds[idx] = sqrt(ssavs[idx]);
				}
			}
		}

		// normalise.  record results as float values, and update type flags after (a bit messy)
		if (useGlobal) {
			logout << "applying global normalising, mean " << _globmean << ", stddev " << _globsd << endl;
		} else {
			logout << "applying separate normalising, mean,stddev: ";

			for (unsigned int idx=0; idx < fTypes.size(); ++idx) {
				int fType = fTypes[idx];
				if (fType == iClass) {
					logout << " {cls}" ;
					continue; // ignore range
				}
				logout << " " << _means[idx] << "," << _sds[idx];
			}
			logout << endl;
		}
		writeBuf();
		_foreach(newFeatures, iter) {
			vector<FValue>& features = *iter;
			for (unsigned int idx=0; idx < features.size(); ++idx) {
				int fType = fTypes[idx];
				double thisVal;
				if (fType == iSeq) thisVal = features[idx].iVal;
				else if (fType == fSeq) thisVal = features[idx].fVal;

				// apply normalising.  record as float
				if (useGlobal) {
					if (_globsd > 0.0) features[idx].fVal = (thisVal - _globmean) / _globsd;
					else features[idx].fVal = 0.0;
				} else {
					if (_sds[idx] > 0.0) features[idx].fVal = (thisVal - _means[idx]) / _sds[idx];
					else features[idx].fVal = 0.0;
				}
			}
		}
		// change all int flags to float
		for (unsigned int idx=0; idx < fTypes.size(); ++idx)
			if (fTypes[idx] == iSeq) fTypes[idx] = fSeq;

	}

	// add to collection
	if (set == trainSet) { // training
		_trainingFt.insert(_trainingFt.end(), newFeatures.begin(), newFeatures.end());
		vector<int>::iterator nIter;
		_foreach(newClasses, nIter) {
			_trainingClasses.push_back(getClassObj(*nIter));
		}

		// record class map
		for (unsigned int i=0; i<newClasses.size(); ++i) {
			int thisClass = newClasses[i];
			_classMap[thisClass].push_back(i);
		}

		logout << "Read classes (Training):" << endl;
		for (unsigned int c=0; c<_nClasses; ++c) {
			logout << c << " ";
			if (! PointRep::_brInstance->_classNames.empty() && PointRep::_brInstance->_classNames.back().size() > c) {
				logout << "(" << PointRep::_brInstance->_classNames.back()[c] <<  ")";
			}
			logout << ": " << _classMap[c].size() << " / " << _trainingFt.size() << " " << _classMap[c].size()  / double(_trainingFt.size())
					<< endl;
		}

		_maxClassSize = 0;
		for (unsigned int c=0; c<_nClasses; ++c) _maxClassSize = _max(_maxClassSize, _classMap[c].size());

		_classDist.clear();
		if (_smallClassMultiplier) {
			logout << "Normalised, small class multiplier:" << _smallClassMultiplier << endl;
			_epochSize = 0;
			map<int, int> counts;
			for (unsigned int c=0; c<_nClasses; ++c) {
				if (_classMap[c].size() < _maxClassSize)
					counts[c] = _classMap[c].size() * _smallClassMultiplier;
				else
					counts[c] = _maxClassSize;

				_epochSize += counts[c];
			}

			for (unsigned int c=0; c<_nClasses; ++c) {
			logout << PointRep::_brInstance->_classNames.back()[c] << "(" << c << "): "
					<< counts[c] << " / " << _epochSize << " " << counts[c] / double(_epochSize)
					<< endl;
				_classDist[getClassObj(c)] = counts[c] / double(_epochSize);
			}
		} else if (_normClasses) {
			logout << "Normalised classes:" << endl;
			// find largest size class for training
			_epochSize = _maxClassSize * _nClasses;
			logout << "Each class size " << _maxClassSize << " (total: " << _epochSize << ")" << endl;
			for (unsigned int c=0; c<_nClasses; ++c) {
				_classDist[getClassObj(c)] = _maxClassSize / double(_epochSize);
			}
		} else {
			_epochSize = _trainingFt.size();
			logout << "Total size " << _epochSize << endl;
			for (unsigned int c=0; c<_nClasses; ++c) {
				_classDist[getClassObj(c)] = _classMap[c].size()  / double(_epochSize);
			}
		}
		if (_epochSampleSize) {
			logout << "Setting epoch size to " << _epochSampleSize << " using repeat or random sampling" << endl;
			writeBuf();
			_epochSize = _epochSampleSize;
		}

		writeBuf();
	} else if (set == testSet) { // test
		_testFt.insert(_testFt.end(), newFeatures.begin(), newFeatures.end());
		vector<int>::iterator nIter;
		_foreach(newClasses, nIter) {
			_testClasses.push_back(getClassObj(*nIter));
		}
		_testSize = _testFt.size();
	} else if (set == predSet) { // prediction set
		_predFt.insert(_predFt.end(), newFeatures.begin(), newFeatures.end());
		_predSize = _predFt.size();
	}

	// record highest class
	for (unsigned int i=0; i<newClasses.size(); ++i) {
		if (newClasses[i] >= _nClasses) _nClasses = newClasses[i] + 1;
	}

	// set binary flag if two classes.  will only be used if in logistic (selSum) mode
	if (_nClasses == 2) {
		_binaryClass = true;

		logout << "Two output classes, setting activation method to tanh (single output)\n";

		ParameterParser::_pinstance->updateParam(INT_NEURMETHOD, "1");
		Params::setupNeurMethod();
		ParameterParser::_pinstance->updateParam(INT_OUTACTMETHOD, "2");
	}

	return true;
}


bool PointWorld::readARFF(string filename, vector<vector<FValue> >& features, vector<int>& classes, bool takeClass)
{
	// ignore all lines starting with '@', just assume all lines have n-1 feature integers and one class (string)
	// load file
    ifstream fileStream;
	trace("attempt to open file %s\n", filename.c_str());
    fileStream.open(filename.c_str());
    if (! fileStream.is_open()) {
    	logout << "could not open file " << filename << endl;
    	writeBufError();
    	return false;
    }

    // read feature types as header is read.  clear first
	vector<int>& featureTypes = PointRep::_brInstance->_featureTypes;
    vector<int>& featureClasses = PointRep::_brInstance->_featureClasses;
    vector<vector<string> >& classNames = PointRep::_brInstance->_classNames;
    featureTypes.clear();
    featureClasses.clear();
    classNames.clear();

    int attributes = 0;

    CIsChar headerPredicate('@');
    CIsSpace spacePredicate;
    CIsChar colPredicate(',');
    vector<string> tokens;
    string thisLine;
    bool indata = false;
    while(fileStream.good()) {
    	getline(fileStream, thisLine);

		// skip comments (lines that start with '%')
		if (thisLine.size() >= 1 && thisLine[0] == '%') continue;

		// check if header entry
    	if (! indata) {
			CTokenizer<CIsChar>::split(tokens, thisLine, headerPredicate);
			if (tokens.size() == 2 && ! tokens[1].empty()) {
				// split columns
				string headerRow = tokens[1];
				CTokenizer<CIsSpace>::split(tokens, headerRow, spacePredicate);
				//CTokenizer<CIsSpace>::tokenize(tokens, headerRow, spacePredicate);

				string operation = tokens[0];
				boost::to_lower(operation);
				//string operation = tokens[0];
				//std::transform(operation.begin(), operation.end(), operation.begin(), ::tolower);

				// find next value
				string restLine = tokens[1];

				// find type
				if (! operation.compare("relation")) {
					//  passed on relation name, just ignore
					string relName = restLine;
				} else if (! operation.compare("attribute")) {
					// tokenise to get attribute label
					CTokenizer<CIsSpace>::tokenize(tokens, restLine, spacePredicate);
					string attrType = tokens[1];
					boost::trim(attrType);
					boost::to_lower(attrType);

					//string attrType = tokens[2];
					//std::transform(attrType.begin(), attrType.end(), attrType.begin(), ::tolower);
					attributes++;
					if (! attrType.compare("real") || ! attrType.compare("numeric")) {
						featureTypes.push_back(fSeq);
						featureClasses.push_back(0);
					} else if (! attrType.compare("int") || ! attrType.compare("integer")) {
						featureTypes.push_back(iSeq);
						featureClasses.push_back(0);
					} else if (attrType.size() >= 1 && attrType[0] == '{') {
						// have brace, read as set of classes
						featureTypes.push_back(iClass);

						// do split on line to get class labels
						CTokenizer<CIsSpace>::split(tokens, restLine, spacePredicate);
						string classLabels = tokens[1];
						boost::trim(classLabels);

						// get labels
						// make sure we have space for class name vector
						classNames.resize(attributes);
						// chop of start and end brace
						string labelString(classLabels, 1, classLabels.size() - 2);

						// get vector of labels
						CTokenizer<CIsChar>::tokenize(tokens, labelString, colPredicate);

						// remove whitespace
						vector<string>::iterator sIter;
						_foreach(tokens, sIter) {
							boost::trim(*sIter);
						}

						// store in classNames vector
						vector<string>& thisClassNames = classNames[attributes-1];
						thisClassNames.assign(tokens.begin(), tokens.end()); // copy tokens

						featureClasses.push_back(tokens.size());
					} else {
						logout << "type for attribute " << featureTypes.size() << " not recognised: " << attrType << endl;
						writeBufWarn();
					}
				} else if (! operation.compare("data")) {
					indata = true;

					// set nClasses from last column
					if (featureTypes.back() != iClass) {
						logout << "last attribute is not class (nominal) type" << endl;
						writeBufWarn();
					}
					_nClasses = featureClasses.back();
				}
			}

			continue;
    	}

        // split columns
        CTokenizer<CIsChar>::tokenize(tokens, thisLine, colPredicate);

        // ignore if we don't have two entries
        if (tokens.size() < 2) continue;

        // grab features (ignore last column)
        int takeCols = (takeClass ? tokens.size() - 1 : tokens.size());
        vector<FValue> ftVec(takeCols);
        int has_unknowns = 0;
        for (unsigned int i=0; i< takeCols; i++) {
			// remove whitespace
			boost::trim(tokens[i]);

        	// convert value and store with appropriate type
        	int fType = featureTypes[i];
        	if (fType == iSeq) ftVec[i].iVal = atoi(tokens[i].c_str());
        	else if (fType == fSeq) ftVec[i].fVal = atof(tokens[i].c_str());
        	else if (fType == iClass) {
        		// look for id of this class label in label vector
        		const vector<string>& labels = classNames[i];
        		bool found = false;
        		for(unsigned int id=0; id<labels.size(); id++) {
        			if (! labels[id].compare(tokens[i])) {
        				ftVec[i].iVal = id;
        				found = true;
        				break;
        			}
        		}
        		// look for '?' symbol, add as new class
    		    if (! found && ! tokens[i].compare("?")) {
    		    	has_unknowns++;
        			//classNames[i].push_back("?");
    				ftVec[i].iVal = MISSINGCLASS; //classNames.size() - 1;
        			found = true;
        		}
    		    if (! found) {
        			logout << "class label " << tokens[i] << " not found, col " << i << endl;
        			writeBufWarn();
        		}
        	}
        }
        if (has_unknowns) {
			logout << "unknown (?) features found: " << has_unknowns << endl;
			writeBufWarn();
        }


        // store
        features.push_back(ftVec);

        // find classification class, assume int
        if (takeClass) {
			int thisClass;
			int lastCol = tokens.size() - 1;
			if (featureTypes[lastCol] == iSeq) thisClass = atoi(tokens[lastCol].c_str());
			else if (featureTypes[lastCol] == iClass) {
				// look for id of this class label in label vector
				const vector<string>& labels = classNames[lastCol];
				bool found = false;
				for(unsigned int id=0; id<labels.size(); id++) {
					if (! labels[id].compare(tokens[lastCol])) {
						thisClass = id;
						found = true;
						break;
					}
				}
				if (! found) {
					logout << "classification label " << tokens[lastCol] << " not found" << endl;
					writeBufWarn();
				}
			} else {
				logout << "unexpected class type " << featureTypes[lastCol] << " for col " << lastCol << ", have " << featureTypes.size() << " types"<< endl;
				writeBufWarn();
			}
			if (thisClass >= _nClasses) _nClasses = thisClass + 1; // assume classes are 0..maxclassid

			classes.push_back(thisClass);
        }

    } // each line

    // trim off last (classification) column of feature types etc
    if (takeClass) {
    	featureTypes.pop_back();
		featureClasses.pop_back();
    }

    return true;
}

bool is_numeric(const std::string& str) {
	std::stringstream conv;
	double tmp;
	conv << str;
	conv >> tmp;
	return conv.eof();
}


bool PointWorld::readCSV(string filename, vector<vector<FValue> >& features, vector<int>& classes)
{
	// ignore all lines starting with '@', just assume all lines have n-1 feature integers and one class (string)
	// load file
    ifstream fileStream;
	trace("attempt to open file %s\n", filename.c_str());
    fileStream.open(filename.c_str());
    if (! fileStream.is_open()) {
    	logout << "could not open file " << filename << endl;
    	writeBufError();
    	return false;
    }

    // read feature types with first entry.  clear first
	vector<int>& featureTypes = PointRep::_brInstance->_featureTypes;
    vector<int>& featureClasses = PointRep::_brInstance->_featureClasses;
    vector<vector<string> >& classNames = PointRep::_brInstance->_classNames;
    featureTypes.clear();
    featureClasses.clear();
    classNames.clear();

    int attributes = 0;

    CIsChar colPredicate(',');
    vector<string> tokens;
    string thisLine;
    bool first = true;
    while(fileStream.good()) {
    	getline(fileStream, thisLine);

        // split columns
        CTokenizer<CIsChar>::tokenize(tokens, thisLine, colPredicate);

        // ignore if we don't have two entries
        if (tokens.size() < 2) continue;

        // interpret column types
        if (first) {
			bool headerDefn = false;
			for (unsigned int i=0; i< tokens.size(); i++) {
				// remove whitespace
				boost::trim(tokens[i]);

				string thisTok = tokens[i];
				boost::to_lower(thisTok);
				if (i==0) {
					// check if this is a header definition
					if (thisTok == "f_x1" || thisTok == "i_x1" || thisTok == "n_x1")
						headerDefn = true;
				}

				if (headerDefn) {
					_Assert(! thisTok.empty(), ("empty header token\n"));
					if (thisTok[0] == 'f') {
						// set type definitions according to number of columns
						featureTypes.push_back(fSeq);
					} else if (thisTok[0] == 'i') {
						// set type definitions according to number of columns
						featureTypes.push_back(iSeq);
					} else if (thisTok[0] == 'n') {
						featureTypes.push_back(iClass);
					}
				} else {
					// headers not defined, need to determine types from observed
					if (is_numeric(tokens[i]) && i < tokens.size() - 1) {
						// treat all numbers as float, otherwise could be int on one line and f elsewhere
						featureTypes.push_back(fSeq);
					} else {
						featureTypes.push_back(iClass);
					}
				}
				// start with no classes (nominals), and add as found
				featureClasses.push_back(0);
			}
			classNames.resize(tokens.size());
	        first = false;
			if (headerDefn) continue;
        }

        // grab features (ignore class column)
        vector<FValue> ftVec(tokens.size() - 1);
        int has_unknowns = 0;
        for (unsigned int i=0; i< tokens.size() - 1; i++) {
        	// remove whitespace
			boost::trim(tokens[i]);

        	// convert value and store with appropriate type
        	int fType = featureTypes[i];
        	if (fType == iSeq) ftVec[i].iVal = atoi(tokens[i].c_str());
        	else if (fType == fSeq) ftVec[i].fVal = atof(tokens[i].c_str());
        	else if (fType == iClass) {
        		// look for id of this class label in label vector
        		const vector<string>& labels = classNames[i];
        		bool found = false;
        		for(unsigned int id=0; id<labels.size(); id++) {
        			if (! labels[id].compare(tokens[i])) {
        				ftVec[i].iVal = id;
        				found = true;
        				break;
        			}
        		}
        		// look for '?' symbol, add as new class
    		    if (! found && ! tokens[i].compare("?")) {
    		    	has_unknowns++;
        			//classNames[i].push_back("?");
    				ftVec[i].iVal = MISSINGCLASS; //classNames.size() - 1;
        			found = true;
        		}
    		    if (! found) {
    		    	// add new class label
    		    	classNames[i].push_back(tokens[i]);
    		    	ftVec[i].iVal = featureClasses[i]++;
        		}
        	}
        }
        if (has_unknowns) {
			logout << "unknown (?) features found: " << has_unknowns << endl;
			writeBufWarn();
        }

        // store
        features.push_back(ftVec);

        // find classification class, assume int
		int thisClass;
		int lastCol = tokens.size() - 1;
		if (featureTypes[lastCol] == iSeq) thisClass = atoi(tokens[lastCol].c_str());
		else if (featureTypes[lastCol] == iClass) {
			// look for id of this class label in label vector
			vector<string>& labels = classNames[lastCol];
			bool found = false;
			for(unsigned int id=0; id<labels.size(); id++) {
				if (! labels[id].compare(tokens[lastCol])) {
					thisClass = id;
					found = true;
					break;
				}
			}
			if (! found) {
		    	// add new class label
		    	thisClass = labels.size();
		    	labels.push_back(tokens[lastCol]);
			}
		} else {
			logout << "unexpected class type " << featureTypes[lastCol] << " for col " << lastCol << ", have " << featureTypes.size() << " types"<< endl;
			writeBufWarn();
		}
		if (thisClass >= _nClasses) _nClasses = thisClass + 1; // assume classes are 0..maxclassid

		classes.push_back(thisClass);

    } // each line

    // re-map all classes, so int values are in order of sorted symbols.  necessary so if first
    // file is presented with symbols in order X,Y,A (given 0,1,2), and the second file is presented
    // in order A,Y,X , the same symbols are given the same index.
	for (unsigned int i=0; i< featureTypes.size(); i++) {
		// only interested in class features
		if (featureTypes[i] != iClass) continue;

		const vector<string>& labels = classNames[i];

		// sort labels
		vector<string> slabels = labels;
		sort(slabels.begin(), slabels.end());

		// get mapping
		tr1::unordered_map<int,int> sMap;
		for (int idx=0; idx<labels.size(); ++idx) {
			const string& sym = labels[idx];

			for (int jidx=0; jidx<slabels.size(); ++jidx) {
				if (slabels[jidx] == sym) {
					sMap[idx] = jidx;
					break;
				}
			}
		}

		// re-map all features for this column
		vector<vector<FValue> >::iterator fIter;
		_foreach(features, fIter)
			(*fIter)[i].iVal = sMap[(*fIter)[i].iVal];
	}

    // trim off last (classification) column of feature types etc
	featureTypes.pop_back();
	featureClasses.pop_back();

    return true;
}

void PointWorld::clearData()
{
	_nClasses = 0;
	_classMap.clear();
	_trainingFt.clear();
	vector<vector<FValue> >().swap(_trainingFt);
	_trainingClasses.clear();
	vector<const Associating*>().swap(_trainingClasses);
	_testFt.clear();
	vector<vector<FValue> >().swap(_testFt);
	_testClasses.clear();
	vector<const Associating*>().swap(_testClasses);
	_predFt.clear();
	vector<vector<FValue> >().swap(_predFt);
}

void PointWorld::reset()
{
}

// could be moved to 'IntClass'
PointClass::PointClass(int value) : _value(value) {}
PointClass::~PointClass() {}
int PointClass::getValue() const
{ return _value; }
void PointClass::display(ostream& os) const
{
	os << _value;
}


// given file name, read images from file
bool readLabels(string filename, vector<int>& labels) {
	// load file
    ifstream fileStream;
	trace("attempt to open file %s\n", filename.c_str());
    fileStream.open(filename.c_str());
    if (! fileStream.is_open()) {
    	logout << "could not open file " << filename << endl;
    	writeBufError();
    	return false;
    }

    // read labels from file

    // first get header
    // magic number first
    int i32 = readI32(fileStream);
    _Assert(i32 == 2049, ("Invalid header reading file %s, expected %d got %d", filename.c_str(), 2049, i32));
    int nLabels = readI32(fileStream);
    logout << "reading " << nLabels << " label records" << endl;
    writeBuf();
    for (int i=0; i<nLabels; ++i) {
    	if (! fileStream.good()) {
    		logout << "error reading from file, expected more data" << endl;
    		writeBufError();
    		return false;
    	}

    	// get label as byte value
    	unsigned char label = fileStream.get();
    	labels.push_back(label);
    }
    return true;
}

// given file name, read labels from file
bool readImages(string filename, vector<vector<FValue> >& features) {
	// load file
    ifstream fileStream;
	trace("attempt to open file %s\n", filename.c_str());
    fileStream.open(filename.c_str());
    if (! fileStream.is_open()) {
    	logout << "could not open file " << filename << endl;
    	writeBufError();
    	return false;
    }

    // read labels from file

    // first get header
    // magic number first
    int i32 = readI32(fileStream);
    _Assert(i32 == 2051, ("Invalid header reading file %s, expected %d got %d", filename.c_str(), 2051, i32));
    int nImages = readI32(fileStream);
    int nRows = readI32(fileStream);
    int nCols = readI32(fileStream);
    logout << "reading " << nImages << " image records, dimensions " << nCols << "x" << nRows << endl;
    writeBuf();

    features.resize(nImages);
    for (int i=0; i<nImages; ++i) {
    	// create new image object
    	features[i].resize(nCols * nRows);

    	// read pixels for this image
    	int fNum = 0;
    	for (int row=0; row<nRows; ++row) {
        	for (int col=0; col<nCols; ++col) {
            	if (! fileStream.good()) {
            		logout << "error reading from file, expected more data" << endl;
            		writeBufError();
            		return false;
            	}

            	// get pixel as byte value
            	//unsigned char pixel = fileStream.get();
            	int pixel = fileStream.get();

            	features[i][fNum++].iVal = pixel;
        	}
    	}

    }

    return true;
}

int readI32(istream& istr) {
	// get 4 bytes and convert to int, assuming MSB first
	unsigned char clist[4];
	clist[0] = istr.get();
	clist[1] = istr.get();
	clist[2] = istr.get();
	clist[3] = istr.get();
	if (! istr.good()) throw READFAIL;

	// appears as though endianness matches, so can just convert
	// directly from read bytes
	int multiplier = 1;
	int i32 = 0;
	for (int i=3; i>=0; --i) {
		i32 += multiplier * clist[i];
		multiplier *= 256;
	}
	return i32;
}
}
