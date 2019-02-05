#ifndef __GENE_ANNOTATION_H__
#define __GENE_ANNOTATION_H__

#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <map>

#include "common.h"
#include "interval_tree.h"

using namespace std;

typedef struct {
	string gene_id;
	string gene_name;
	string chr;
	uint32_t start;
	uint32_t end;
} ExonSeg;

typedef struct {
	uint32_t start;
	uint32_t end;
	uint32_t next_start;
	uint32_t prev_end;
	uint32_t trans_id_int;
	int exon_num_int;

	bool forward_strand;

	string chr;
	string source;
	string type;
	string gene_id;
	string trans_id;
	string exon_num;
	string gene_name;
} GTFRecord;

typedef struct {
	string contig;
	uint32_t shift;
} ConShift;

class GTFParser {
private:
	FILE* input;
	size_t file_size;

	char* line;
	int len;
	size_t max_line_size;

	map <string, map <UniqSeg, string> > merged_exons; 
	map <string, map <GeneInfo, string> > merged_genes; 

	map <string, FlatIntervalTree <UniqSeg> > exons_int_map;
	map <string, FlatIntervalTree <GeneInfo> > genes_int_map;
	
	map <string, map <string, GeneInfo> > gid2ginfo;

	map <string, ConShift> chr2con;
	map <string, vector<ConShift> > con2chr;
	map <string, int> level;

	map <string, vector <int> > trans_start_ind;

	void set_contig_shift(const ContigLen* contig_len, int contig_count);
	void chrloc2conloc(string& chr, uint32_t& start, uint32_t& end);

public:
	map <string, vector <string> > transcript_ids;
	map <string, vector < vector <uint8_t> > > trans2seg;

	GTFParser (void);
	GTFParser (char* filename, const ContigLen* contig_len, int contig_count);
	~GTFParser (void);

	void init (char* filename, const ContigLen* contig_len, int contig_count);
	bool get_next (void);
	bool has_next (void);
	bool read_next (void);

	void tokenize(char* line, int len, const string& delim, vector<string>& gtf_fields);
	bool parse_gtf_rec (char* line, int len, GTFRecord* cr);
	bool load_gtf (void);

	int binary_search(const vector <ExonSeg>& seg, int beg, int end, bool on_start, uint32_t target);

	uint32_t get_upper_bound       (uint32_t spos, uint32_t mlen, uint32_t rlen, uint32_t& max_end, const IntervalInfo<UniqSeg>*& ol_exons);
	uint32_t get_upper_bound_lookup(uint32_t spos, uint32_t mlen, uint32_t rlen, uint32_t& max_end, const IntervalInfo<UniqSeg>*& ol_exons);

	void get_upper_bound_alu(uint32_t spos, uint32_t mlen, uint32_t rlen, JunctionDist& jd);
	
	const IntervalInfo<UniqSeg>* get_location_overlap    (uint32_t loc, bool use_mask);
	const IntervalInfo<UniqSeg>* get_location_overlap_ind(uint32_t loc, bool use_mask, int& ind);
	const IntervalInfo<GeneInfo>* get_gene_overlap       (uint32_t loc, bool use_mask);

	uint32_t get_interval_epos(int interval_ind);
	const IntervalInfo<UniqSeg>* get_interval(int interval_ind);

	ConShift get_shift(const string& contig, uint32_t loc);

	GeneInfo* get_gene_info(const string& gid);

	int get_trans_start_ind(const string& contig, uint32_t tid);

	void print_record(const GTFRecord& r);
};

// match an interval:
// [ spos, spos + mlen )
// spos: Start POSition of matched region
// mlen: Matched LENgth
// rlen: the lenght of the read remained to be matched (Rmained LENgth)
inline uint32_t GTFParser::get_upper_bound(uint32_t spos, uint32_t mlen, uint32_t rlen, uint32_t& max_end, const IntervalInfo<UniqSeg>*& ol_exons) {
	if (near_border[contigNum][spos] & 1) {		// if not near any border, skip lookup
		return get_upper_bound_lookup(spos, mlen, rlen, max_end, ol_exons);
	}
	else {
		// fprintf(stderr, "skip lookup\n");
		max_end = 0;
		ol_exons = NULL;
		return spos + rlen + EDTH;	// allowing deletion of size at most "EDTH"
	}
}

inline uint32_t GTFParser::get_interval_epos(int interval_ind) {
	return exons_int_map[contigName].get_node(interval_ind)->epos;
}

inline const IntervalInfo<UniqSeg>* GTFParser::get_interval(int interval_ind) {
	return exons_int_map[contigName].get_node(interval_ind);
}

inline int GTFParser::get_trans_start_ind(const string& contig, uint32_t tid) {
	return trans_start_ind[contig][tid];
}

inline GeneInfo* GTFParser::get_gene_info(const string& gid) {
	return &gid2ginfo[contigName][gid];
}

extern GTFParser gtf_parser;

#endif	//__GENE_ANNOTATION_H__
