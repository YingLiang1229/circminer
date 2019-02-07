#include <cstring>

#include "process_circ.h"
#include "gene_annotation.h"
#include "hash_table.h"
#include "fastq_parser.h"
#include "match_read.h"
#include "chain.h"

#define BINSIZE 5000

ProcessCirc::ProcessCirc (int last_round_num, int ws) {
	sprintf(fq_file1, "%s_%d_remain_R1.fastq", outputFilename, last_round_num);
	sprintf(fq_file2, "%s_%d_remain_R2.fastq", outputFilename, last_round_num);

	fprintf(stdout, "%s\n",fq_file1 );
	window_size = ws;
	step = 3;
	regional_ht.init(ws);
}

ProcessCirc::~ProcessCirc (void) {

}

void ProcessCirc::do_process (void) {
	/**********************/
	/**Loading Hash Table**/
	/**********************/

	double cputime_start = get_cpu_time();
	double realtime_start = get_real_time();
	double cputime_curr;
	double realtime_curr;

	double tmpTime;
	int	flag;
	checkSumLength = (WINDOW_SIZE > kmer) ? 0 : kmer - WINDOW_SIZE;

	char index_file [FILE_NAME_LENGTH];
	strcpy(index_file, referenceFilename);
	strcat(index_file, ".index");

	initCommon();
	
	THREAD_COUNT = threads;
	fprintf(stdout, "# Threads: %d\n", THREAD_COUNT);
	for (int i = 0; i < 255; i++)
		THREAD_ID[i] = i;

	if (!checkHashTable(index_file))
		return;

	ContigLen* orig_contig_len;
	int contig_cnt;
	if (!initLoadingHashTableMeta(index_file, &orig_contig_len, &contig_cnt))
		return;

	fprintf(stdout, "Started loading index...\n");

	flag = loadHashTable ( &tmpTime );  			// Reading a fragment

	cputime_curr = get_cpu_time();
	realtime_curr = get_real_time();

	fprintf(stdout, "[P] Loaded genome index successfully in %.2lf CPU sec (%.2lf real sec)\n\n", cputime_curr - cputime_start, realtime_curr - realtime_start);

	cputime_start = cputime_curr;
	realtime_start = realtime_curr;

	/**********************/
	/**Finised Loading HT**/
	/**********************/

	/*******************/
	/**GTF Parser Init**/
	/*******************/

	gtf_parser.init(gtfFilename, orig_contig_len, contig_cnt);
	if (! gtf_parser.load_gtf()) {
		fprintf(stdout, "Error in reading GTF file.\n");
		exit(1);
	}
	else 
		fprintf(stdout, "GTF file successfully loaded!\n");

	cputime_curr = get_cpu_time();
	realtime_curr = get_real_time();

	fprintf(stdout, "[P] Loaded GTF in %.2lf CPU sec (%.2lf real sec)\n\n", cputime_curr - cputime_start, realtime_curr - realtime_start);

	cputime_start = cputime_curr;
	realtime_start = realtime_curr;

	/*******************/
	/**Finished GTF PI**/
	/*******************/

	double fq_cputime_start = cputime_curr;
	double fq_realtime_start = realtime_curr;

	contigName = getRefGenomeName();

	bool is_pe = pairedEnd;

	FASTQParser fq_parser1(fq_file1);
	Record* current_record1;

	FASTQParser fq_parser2;
	Record* current_record2;
	if (is_pe) {
		fq_parser2.init(fq_file2);
	}
	
	int line = 0;
	while ( (current_record1 = fq_parser1.get_next()) != NULL ) { // go line by line on fastq file
		if (is_pe)
			current_record2 = fq_parser2.get_next();
		if (current_record1 == NULL)	// no new line
			break;

		line++;
		vafprintf(2, stderr, "Line: %d\n", line);

		if (line % LINELOG == 0) {
			cputime_curr = get_cpu_time();
			realtime_curr = get_real_time();

			fprintf(stdout, "[P] %d reads in %.2lf CPU sec (%.2lf real sec)\t Look ups: %u\n", line, cputime_curr - cputime_start, realtime_curr - realtime_start, lookup_cnt);
			fflush(stdout);

			cputime_start = cputime_curr;
			realtime_start = realtime_curr;
			
			lookup_cnt = 0;
		}

		call_circ(current_record1, current_record2);
	}

	cputime_curr = get_cpu_time();
	realtime_curr = get_real_time();

	fprintf(stdout, "[P] Mapping in %.2lf CPU sec (%.2lf real sec)\n\n", cputime_curr - fq_cputime_start, realtime_curr - fq_realtime_start);
}

// PE
void ProcessCirc::call_circ(Record* current_record1, Record* current_record2) {
	MatchedRead mr = *(current_record1->mr);
	vafprintf(2, stderr, "%s\n%s\n", current_record1->seq, current_record2->seq);
	vafprintf(2, stderr, "%s\t%s\t%u\t%u\t%d\t%u\t%u\t%d\t%s\t%u\t%u\t%d\t%u\t%u\t%d\t%d\t%d\t%d\t%d\n", 
									current_record1->rname, 
									mr.chr_r1.c_str(), mr.spos_r1, mr.epos_r1, mr.mlen_r1, mr.qspos_r1, mr.qepos_r1, mr.ed_r1, 
									mr.chr_r2.c_str(), mr.spos_r2, mr.epos_r2, mr.mlen_r2, mr.qspos_r2, mr.qepos_r2, mr.ed_r2,
									mr.tlen, mr.junc_num, mr.gm_compatible, mr.type);

	bool r1_partial = mr.mlen_r1 < mr.mlen_r2;
	char* remain_seq = (r1_partial) ? ((mr.r1_forward) ? current_record1->seq : current_record1->rcseq) : 
									  ((mr.r2_forward) ? current_record2->seq : current_record2->rcseq) ;
	uint32_t qspos = (r1_partial) ? (((mr.qspos_r1 - 1) > (current_record1->seq_len - mr.qepos_r1)) ? (1) : (mr.qepos_r1 + 1)) :
									(((mr.qspos_r2 - 1) > (current_record2->seq_len - mr.qepos_r2)) ? (1) : (mr.qepos_r2 + 1)) ;

	uint32_t qepos = (r1_partial) ? (((mr.qspos_r1 - 1) > (current_record1->seq_len - mr.qepos_r1)) ? (mr.qspos_r1 - 1) : (current_record1->seq_len)) :
									(((mr.qspos_r2 - 1) > (current_record2->seq_len - mr.qepos_r2)) ? (mr.qspos_r2 - 1) : (current_record2->seq_len)) ;

	if (qepos < qspos) {	// it was fully mapped
		return;
	}

	const IntervalInfo<GeneInfo>* gene_info = gtf_parser.get_gene_overlap(mr.spos_r1, false);
	bool found = (gene_info != NULL);
	if (! found) {
		vafprintf(2, stderr, "Gene not found!\n");
		return;
	}
	vafprintf(2, stderr, "# Gene overlaps: %d\n", gene_info->seg_list.size());

	for (int i = 0; i < gene_info->seg_list.size(); i++) {
		uint32_t gene_len = gene_info->seg_list[i].end - gene_info->seg_list[i].start + 1;
		char gene_seq[gene_len + 1];
		gene_seq[gene_len] = '\0';
		pac2char(gene_info->seg_list[i].start, gene_len, gene_seq);
		//vafprintf(2, stderr, "Gene: %s\n", gene_seq);

		regional_ht.create_table(gene_seq, 0, gene_len);

		vafprintf(2, stderr, "R%d partial: [%d-%d]\n", (int) (!r1_partial) + 1, qspos, qepos);
		vafprintf(2, stderr, "%s\n", remain_seq);

		// for (int i = qspos - 1; i <= qepos - window_size; i += 3) {
		// 	GIList* gl = regional_ht.find_hash(regional_ht.hash_val(remain_seq + i));
		// 	if (gl == NULL) {
		// 		vafprintf(2, stderr, "Hash val not found!!!\n");
		// 	}
		// 	vafprintf(2, stderr, "Occ: %d\n", gl->cnt);
		// 	for (int j = 0; j < gl->cnt; j++) {
		// 		vafprintf(2, stderr, "%d\t", gl->locs[j].info);
		// 	}
		// 	vafprintf(2, stderr, "\n");

		// }

		//binning(qspos, qepos, regional_ht, remain_seq, gene_len);
		uint32_t rspos, repos;
		chaining(qspos, qepos, regional_ht, remain_seq, gene_len, gene_info->seg_list[i].start, rspos, repos);

		vafprintf(2, stderr, "Coordinates: [%d-%d]\n", rspos, repos);

		if (!(rspos == 0 and repos == 0)) { // found valid chaining
			//check_circ_validity();
			uint32_t start_bp;
			uint32_t end_bp;
			if (qspos == 1) {
				start_bp = (r1_partial) ? mr.spos_r1 : mr.spos_r2;
				end_bp = repos;
			}
			else {
				start_bp = rspos;
				end_bp = (r1_partial) ? mr.epos_r1 : mr.epos_r2;
			}
			fprintf(stderr, "%s\t%s\t%d\t%d\n", current_record1->rname, mr.chr_r1.c_str(), start_bp, end_bp);
		}

	}
}

void ProcessCirc::binning(uint32_t qspos, uint32_t qepos, const RegionalHashTable& regional_ht, char* remain_seq, uint32_t gene_len) {
	int bin_num = gene_len / BINSIZE + 1;
	int bins[bin_num];
	int max_id = 0;
	memset(bins, 0, bin_num * sizeof(int));

	for (int i = qspos - 1; i <= qepos - window_size; i += step) {
		GIMatchedKmer* gl = regional_ht.find_hash(regional_ht.hash_val(remain_seq + i));
		if (gl == NULL) {
			vafprintf(2, stderr, "Hash val not found!!!\n");
		}

		vafprintf(2, stderr, "Occ: %d\n", gl->frag_count);
		
		for (int j = 0; j < gl->frag_count; j++) {
			int bin_id = gl->frags[j].info / BINSIZE;
			bins[bin_id]++;

			if (bins[bin_id] > bins[max_id])
				max_id = bin_id;

			vafprintf(2, stderr, "%d - %d\t", gl->frags[j].info, bins[bin_id]);
		}
		vafprintf(2, stderr, "\n");
	}	
	vafprintf(2, stderr, "Biggest bin: bin[%d][%d - %d] = %d\n", max_id, max_id * BINSIZE, (max_id+1) * BINSIZE - 1, bins[max_id]);

}

void ProcessCirc::chaining(uint32_t qspos, uint32_t qepos, const RegionalHashTable& regional_ht, char* remain_seq, uint32_t gene_len, uint32_t shift, uint32_t& rspos, uint32_t& repos) {
	int seq_len = qepos - qspos + 1;
	int kmer_cnt = ((qepos - qspos + 1) - window_size) / step + 1;
	GIMatchedKmer* fl = (GIMatchedKmer*) malloc(kmer_cnt * sizeof(GIMatchedKmer));
	// Initialize
	for (int i = 0; i < kmer_cnt; i++)
		(fl + i)->frag_count = 0;

	int l = 0;
	for (int i = qspos - 1; i <= qepos - window_size; i += step) {
		GIMatchedKmer* gl = regional_ht.find_hash(regional_ht.hash_val(remain_seq + i));
		// copy from gl to fl[j]
		fl[l].qpos = i;
		if (gl == NULL) {
			fl[l].frag_count = 0;
			fl[l].frags = NULL;
			l++;
			vafprintf(2, stderr, "Hash val not found!!!\n");
			continue;
		}

		fl[l].frag_count = gl->frag_count;
		fl[l].frags = gl->frags;

		vafprintf(2, stderr, "Occ: %d\n", fl[l].frag_count);
		
		for (int j = 0; j < fl[l].frag_count; j++) {
			fl[l].frags[j].info += shift;
			vafprintf(2, stderr, "%d\t", fl[l].frags[j].info);
		}
		vafprintf(2, stderr, "\n");
		l++;
	}

	chain_list bc;
	bc.chains = (chain_t*) malloc(BESTCHAINLIM * sizeof(chain_t));
	for (int i = 0; i < BESTCHAINLIM; i++)
		bc.chains[i].frags = (fragment_t*) malloc(kmer_cnt * sizeof(fragment_t));

	chain_seeds_sorted_kbest2(qepos, fl, bc, window_size, kmer_cnt);

	vafprintf(1, stderr, "Chaining score:%.4f,\t len: %lu\n", bc.chains[0].score, (unsigned long)bc.best_chain_count);

	int allowed_missed_kmers = (qepos - qspos + 1) / 20 * 3 + 1;
	vafprintf(2, stderr, "Allowed missing kmers: %d\n", allowed_missed_kmers);

	rspos = 0;
	repos = 0;
	uint32_t curr_rspos, curr_repos;
	int least_miss = INF;
	int least_miss_ind = -1;
	int missing;
	for (int j = 0; j < bc.best_chain_count; j++) {
		missing = kmer_cnt - bc.chains[j].chain_len;
		vafprintf(2, stderr, "Actual missing: %d\n", missing);
		if (missing > least_miss)
			break;

		curr_rspos = bc.chains[j].frags[0].rpos - bc.chains[j].frags[0].qpos;
		curr_repos = bc.chains[j].frags[bc.chains[j].chain_len - 1].rpos + (seq_len - 1 - bc.chains[j].frags[bc.chains[j].chain_len - 1].qpos);
		uint32_t curr_qlen = bc.chains[j].frags[bc.chains[j].chain_len - 1].qpos - bc.chains[j].frags[0].qpos;
		uint32_t qlen = (least_miss_ind != -1) ? bc.chains[least_miss_ind].frags[bc.chains[least_miss_ind].chain_len - 1].qpos - bc.chains[least_miss_ind].frags[0].qpos : 0;
		if (missing < least_miss or (missing == least_miss and curr_qlen > qlen)) {
			least_miss = missing;
			least_miss_ind = j;
			rspos = curr_rspos;
			repos = curr_repos;
		}

		for (int i = 0; i < bc.chains[j].chain_len; i++) {
			vafprintf(1, stderr, "#%d\tfrag[%d]: %lu\t%d\t%d\n", j, i, bc.chains[j].frags[i].rpos, bc.chains[j].frags[i].qpos, bc.chains[j].frags[i].len);
		}
	}

	for (int i = 0; i < BESTCHAINLIM; i++)
		free(bc.chains[i].frags);

	free(bc.chains);

}


int ProcessCirc::get_exact_locs_hash (char* seq, uint32_t qspos, uint32_t qepos) {

}