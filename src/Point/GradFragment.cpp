/*
 * GradFragment.cpp
 *
 *  Created on: 27/05/2014
 *      Author: anthony
 */

#include "PointRep.h"

namespace pointrep {

GradFragment::GradFragment(PtHarness& parent)
	: PtFragment(parent)
{

}

GradFragment::~GradFragment() {

}


bool GradFragment::operator==(const Fragment& frag) {
	// don't allow merging
	return false;
}

bool GradFragment::findActivation(InstInfo& t) const
{
	PtInstInfo& bt = static_cast<PtInstInfo&>(t);

	const vector<FValue>& rString = bt._features;
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;

	// single index, check activation value
	double activation = Params::_MINTARGET;
	int matchValue = 0;
	if (! rString.empty()) {

		// find output value
		double sum = 0.0;
		double fVal;
		for (int n=0; n<_operations.size(); ++n) {
			if (_operations[n] == eq) sum += 1.0;
			int idx = _indexes[n];

			if (fTypes[idx] == iClass) {
				if (rString.at(idx).iVal == _values[n].iVal) sum += 1.0;
				continue;
			}
			if (fTypes[idx] == iSeq) fVal = rString.at(idx).iVal;
			else if (fTypes[idx] == fSeq) fVal = rString.at(idx).fVal;

			sum += _weightVal[n]._val * fVal;
		}

		// find activation value
		activation = NeurFn(sum + _biasParam._val);

		if (activation >= PointRep::_brInstance->_matchThreshold) {
			matchValue = 1;
		}
	}

	t._fragDetails[this]._match = matchValue;
	t._fragDetails[this]._activation = activation;

	return matchValue;
}


void GradFragment::createFeatures(InstInfo& t, bool forceRnd) {
	// from the current world state, produce pattern
	int method = static_cast<GradHarness*>(_parHarness)->_gradCreateMethod;
	if (method == gradThresh) initThresh(t, forceRnd);
	else if (method == gradRandom) initRnd(t);
}

void GradFragment::initRnd(InstInfo& t) {
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;

	int numWts = _RandOf(PointRep::_brInstance->_atomSymbols) + 1;
	_indexes.resize(numWts);
	_values.resize(numWts);
	_operations.resize(numWts);
	_weightVal.resize(numWts);
	_biasParam._val = _gRand(SMRNDSD);

	const vector<FValue>& vec = static_cast<PtInstInfo&>(t)._features;
	vector<double> fts(numWts);
	for (int n=0; n<numWts; ++n) {
		int idx = _RandOf(fTypes.size());
		_indexes[n] = idx;
		if (fTypes[idx] == iClass) {
			_operations[n] = eq;
			if (! vec.empty()) _values[n] = vec.at(idx);
			else {
				const vector<int>& fClasses = PointRep::_brInstance->_featureClasses;
				_values[n].iVal = _RandOf(fClasses[idx]);
			}
			continue;
		} else {
			// grab from state to match feature
			// define sigmoid activation
			_operations[n] = other;
			double val;
			if (! vec.empty()) {
				if (fTypes[idx] == iSeq) val = vec.at(idx).iVal;
				else if (fTypes[idx] == fSeq) val = vec.at(idx).fVal;
			} else {
				vector<pair<FValue, FValue> >& fRanges = PointRep::_brInstance->_featureRanges;
				double min, max;
				if (fTypes[idx] == iSeq) {
					min = fRanges[idx].first.iVal;
					max = fRanges[idx].second.iVal;
				} else {
					min = fRanges[idx].first.fVal;
					max = fRanges[idx].second.fVal;
				}

				val = _fRand() * (max - min) + min;
			}
			fts[n] = val;

			// initialise random
			_weightVal[n]._val = _gRand(SMRNDSD);
		}
	}

	// adjust so gives positive response
	int ttl = MAXADJITER;
	double act, sum = 0.0;
	for (int n=0; n<numWts; ++n) {
		if (_operations[n] == eq) sum += 1.0;
		sum += _weightVal[n]._val * fts[n];
	}
	act = NeurFn(sum + _biasParam._val);

	//double margin = 0.1 * (Params::_MAXTARGET - Params::_MINTARGET);
	double target = (Params::_MAXTARGET - PointRep::_brInstance->_matchThreshold) / 2.0;
	double margin = (target - PointRep::_brInstance->_matchThreshold) / 2.0;
	double delta = act - target;
	while (fabs(delta) > margin && ttl--) {
		//double delta = act - Params::_MAXTARGET;
		double grad = delta * NeurDelta(act);
		updateWts(grad, _biasParam, FASTLR);
		for (int n=0; n<numWts; ++n) {
			if (_operations[n] != other) continue;
			updateWts(grad * fts[n], _weightVal[n], FASTLR);
		}

		// find output again
		sum = 0.0;
		for (int n=0; n<numWts; ++n) {
			if (_operations[n] == eq) sum += 1.0;
			sum += _weightVal[n]._val * fts[n];
		}
		act = NeurFn(sum + _biasParam._val);
		delta = act - target;
	}

}
void GradFragment::initThresh(InstInfo& t, bool forceRnd) {
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;
	double buf = PointRep::_brInstance->_activeBuffer;

	bool usePositive = _RandOf(2); // have +ve half -ve
	_indexes.resize(1);
	_values.resize(1);
	_operations.resize(1);
	_weightVal.resize(1);
	const vector<FValue>& vec = static_cast<PtInstInfo&>(t)._features;
	if (! vec.empty() && ! forceRnd) {
		// grab from state to match feature


		int idx = _RandOf(vec.size());
		_indexes[0] = idx;
		if (fTypes[idx] == iClass) {
			_operations[0] = eq;
			_values[0] = vec.at(idx);
		} else {
			// define sigmoid activation

			double val;
			if (fTypes[idx] == iSeq) val = vec.at(idx).iVal;
			else if (fTypes[idx] == fSeq) val = vec.at(idx).fVal;

			// define using y = tanh((x - (v - c)) / c), tanh(-(x - (v - c)) / c)
			double c = buf / (1.0 - GradHarness::_THRESHSUMVAL);
			if (usePositive) {
				_weightVal[0]._val = 1.0 / c;
				_biasParam._val = - (val/c) + 1.0;
			} else {
				_weightVal[0]._val = - 1.0 / c;
				_biasParam._val = (val/c) + 1.0;
			}
		}
	} else {
		// world present but not active, generate random
		_indexes.resize(1);
		_values.resize(1);
		_operations.resize(1);
		const vector<pair<FValue,FValue> >& ranges = PointRep::_brInstance->_featureRanges;
		const vector<int>& fClasses = PointRep::_brInstance->_featureClasses;
		int idx = _RandOf(fTypes.size());
		_indexes[0] = idx;

		// choose operation and value within range
		if (fTypes[idx] == iClass) {
			_operations[0] = eq;
			_values[0].iVal = _RandOf(fClasses[idx]);
		} else if (fTypes[idx] == iSeq || fTypes[idx] == fSeq) {
			const pair<FValue,FValue>& rng = ranges[idx];
			double val;
			if (fTypes[idx] == iSeq) {
				val = _RandOf(rng.second.iVal - rng.first.iVal) + rng.first.iVal;
			} else if (fTypes[idx] == fSeq) {
				val = _fRand() * (rng.second.fVal - rng.first.fVal) + rng.first.fVal;
			}

			// define sigmoid activation
			// define using y = tanh((x - (v - c)) / c), tanh(-(x - (v - c)) / c)

			// should this be incorporated?: double c = buf / (1.0 - GradHarness::_THRESHSUMVAL);
			if (usePositive) {
				_weightVal[0]._val = 1.0 / buf;
				_biasParam._val = - (val - buf) / buf;
			} else {
				_weightVal[0]._val = - 1.0 / buf;
				_biasParam._val = (val + buf) / buf;
			}
		}
	}
}

void GradFragment::display(ostream& outstream) const {
	Fragment::display(outstream);

	if (_indexes.empty()) return;

	outstream << " ";
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;

	for (int i=0; i<_indexes.size(); ++i) {
		if (! i) outstream << ",";
		int idx = _indexes[i];
		outstream << idx << ":";
		if (fTypes[idx] == iClass) { outstream << "==" << _values[i].iVal; }
		else {
			outstream << "m=" << _weightVal[i]._val;
		}
	}
	outstream << ",b=" << _biasParam._val;
}

/*NeurParam& GradFragment::getWeightParam()
{
	return _weightVal;
}*/

void GradFragment::write(ostream& out) const
{
	PtFragment::write(out);

	out << " " << _weightVal.size();
	for(int i=0; i<_weightVal.size(); ++i) {
		out << " " << _weightVal[i]._val << " " << _weightVal[i]._mom;
	}
}
void GradFragment::read(istream& in)
{
	PtFragment::read(in);

	int n;
	in >> n;
	_weightVal.resize(n);
	for(int i=0; i<n; ++i) {
		in >> _weightVal[i]._val >> _weightVal[i]._mom;
	}
}

void GradFragment::doBackprop(InstInfo& t, double delta)
{
	if (_indexes.empty()) return;

	PtInstInfo& bt = static_cast<PtInstInfo&>(t);
	const vector<FValue>& rString = bt._features;
	const vector<int>& fTypes = PointRep::_brInstance->_featureTypes;

	tr1::unordered_map<NeurParam*, vector<double> >& pMap = bt._fragDetails[this]._paramChanges;
	pMap[&_biasParam].push_back(delta);
	//updateWts(delta, _biasParam);
	for (int n=0; n<_indexes.size(); ++n) {
		if (_operations[n] != other) continue;
		int idx = _indexes[n];
		// update single parameter
		double val;
		if (fTypes[idx] == iSeq) val = rString[idx].iVal;
		else if (fTypes[idx] == fSeq) val = rString[idx].fVal;
		pMap[&(_weightVal[n])].push_back(delta);
		//updateWts(delta * val, _weightVal[n]);
	}

}


} // namespace
