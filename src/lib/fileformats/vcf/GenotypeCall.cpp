#include "GenotypeCall.hpp"

#include "common/Tokenizer.hpp"

#include <boost/format.hpp>
#include <limits>

using boost::format;

BEGIN_NAMESPACE(Vcf)

GenotypeCall GenotypeCall::Null;
GenotypeIndex GenotypeIndex::Null{std::numeric_limits<GenotypeIndex::value_type>::max()};

GenotypeCall::GenotypeCall()
    : _phased(false)
{
}

GenotypeCall::GenotypeCall(const std::string& call)
    : _phased(false)
    , _string(call)
{
    Tokenizer<std::string> tok(call, "|/");
    GenotypeIndex idx;

    // note: a delimiter of | denotes phased data
    // if anything is phased, we treat the whole genotype as phased
    // hopefully, mixing of phased and unphased data in a single
    // call isn't meaningful...
    while (!tok.eof()) {
        if (tok.nextTokenMatches(".")) {
            idx = GenotypeIndex::Null;
            tok.advance();
        }
        else if (!tok.extract(idx.value)) {
            throw std::runtime_error(str(format("Genotype parse error "
                "(GT=%1%): expected a number or '.'"
                ) % call));
        }

        _phased |= tok.lastDelim() == '|';
        _indices.push_back(idx);
        _indexSet.insert(idx);
    }
}

bool GenotypeCall::empty() const {
    return _indices.empty();
}

GenotypeCall::size_type GenotypeCall::size() const {
    return _indices.size();
}

GenotypeCall::const_iterator GenotypeCall::begin() const {
    return _indices.begin();
}

GenotypeCall::const_iterator GenotypeCall::end() const {
    return _indices.end();
}

bool GenotypeCall::phased() const {
    return _phased;
}

bool GenotypeCall::heterozygous() const {
    return diploid() && _indexSet.size() == 2;
}

bool GenotypeCall::homozygous() const {
    return diploid() && _indexSet.size() == 1;   //if only one unique allele then homozygous
}

bool GenotypeCall::diploid() const {
    return _indices.size() == 2;
}

bool GenotypeCall::reference() const {
    return _indexSet.size() == 1 && _indexSet.count(GenotypeIndex{0});
}

const GenotypeIndex& GenotypeCall::operator[](size_type idx) const {
    return _indices[idx];
}

const vector<GenotypeIndex>& GenotypeCall::indices() const {
    return _indices;
}

const set<GenotypeIndex>& GenotypeCall::indexSet() const {
    return _indexSet;
}

const string& GenotypeCall::string() const {
    return _string;
}

bool GenotypeCall::operator==(const GenotypeCall& rhs) const {
    if(_phased == rhs._phased) {
        if(_phased) {
            return (_string == rhs._string);
        }
        else {
            //order doesn't matter so need to sort
            return (_indexSet == rhs._indexSet);
        }
    }
    else {
        return false;
    }
}

bool GenotypeCall::operator!=(const GenotypeCall& rhs) const {
    return !(*this == rhs);
}

bool GenotypeCall::operator<(const GenotypeCall& rhs) const {
    // we'll say phased is smaller than unphased
    if (_phased != rhs._phased)
        return _phased;

    if (_indexSet.size() < rhs._indexSet.size())
        return true;

    if (_indexSet.size() > rhs._indexSet.size())
        return false;

    if (_phased) {
        return _string < rhs._string;
    }

    if (!_indexSet.empty()) {
        // indexSets are the same size
        auto ia = _indexSet.begin();
        auto ib = rhs._indexSet.begin();
        while (ia != _indexSet.end() && ib != _indexSet.end()) {
            if (*ia < *ib)
                return true;
            ++ia;
            ++ib;
        }
    }
    return false;
}

END_NAMESPACE(Vcf)

std::ostream& operator<<(std::ostream& os, Vcf::GenotypeIndex const& gtidx) {
    if (gtidx == Vcf::GenotypeIndex::Null)
        os << '.';
    else
        os << gtidx.value;
    return os;
}

std::ostream& operator<<(std::ostream& os, Vcf::GenotypeCall const& gt) {
    os << gt.string();
    return os;
}
