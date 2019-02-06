#include <set>
#include <cstdlib>
#include <stdint.h>

#include "extend.h"
#include "common.h"
#include "align.h"
#include "match_read.h"
#include "gene_annotation.h"

void extend_right_trans(uint32_t tid, uint32_t pos, char* ref_seq, int ref_len, char* qseq, int qseq_len, 
						int ed_th, uint32_t ub,  AlignRes& best, bool& consecutive, map <AllCoord, AlignRes>& align_res);
void extend_left_trans (uint32_t tid, uint32_t pos, char* ref_seq, int ref_len, char* qseq, int qseq_len, 
						int ed_th, uint32_t lb,  AlignRes& best, bool& consecutive, map <AllCoord, AlignRes>& align_res);


// pos is exclusive
// [ pos+1, pos+len ]
bool extend_right(const vector <uint32_t>& common_tid, char* seq, uint32_t& pos, int len, int ed_th, uint32_t ub, 
					AlignRes& best_alignment) {
	int seq_len = len;
	int ref_len = len + INDELTH;
	uint32_t orig_pos = pos;

	char ref_seq[maxReadLength + 4 * INDELTH];
	ref_seq[ref_len] = '\0';
	
	int indel;
	bool consecutive = false;
	AlignRes curr_alignment(ub);
	best_alignment.set(pos, ed_th + 1, maxSc + 1, INDELTH + 1, 0);

	map <AllCoord, AlignRes> align_res; 
	
	for (int i = 0; i < common_tid.size(); i++) {
		extend_right_trans(common_tid[i], pos, ref_seq, ref_len, seq, seq_len, ed_th, ub, best_alignment, consecutive, align_res);
		//best_alignment.print();
		// if (best_alignment.qcovlen >= seq_len and best_alignment.ed == 0 and best_alignment.sclen == 0) {
		// 	pos = best_alignment.pos - best_alignment.sclen;
		// 	return true;
		// }
	}

	uint32_t best_rmpos = best_alignment.pos;
	int min_ed = best_alignment.ed;
	int sclen_best = best_alignment.sclen;

	//vafprintf(2, stderr, "Min Edit Dist: %d\tNew RM POS: %u\tCovered len: %d\n", min_ed, best_rmpos, best_alignment.rcovlen);

	if (min_ed <= ed_th) {
		pos = best_rmpos - sclen_best;
		vafprintf(2, stderr, "Min Edit Dist: %d\tNew RM POS: %u\n", min_ed, pos);
		if (best_alignment.qcovlen >= seq_len)
			return true;
	}
	
	if (consecutive)
		return false;

	// intron retention
	if (!pac2char(orig_pos + 1, ref_len, ref_seq))
		return false;

	min_ed = alignment.local_alignment_right_sc(ref_seq, ref_len, seq, seq_len, sclen_best, indel);
	
	vafprintf(2, stderr, "Intron Retention:\nrmpos: %lu\textend len: %d\n", orig_pos, len);
	vafprintf(2, stderr, "str beg str:  %s\nread beg str: %s\nedit dist %d\n", ref_seq, seq, min_ed);

	if (min_ed <= ed_th) {
		curr_alignment.set(orig_pos + seq_len - indel, min_ed, sclen_best, indel, seq_len);
		best_alignment.update_right(curr_alignment);
		pos = orig_pos + seq_len - indel - sclen_best;
		vafprintf(2, stderr, "Intron Retention: Min Edit Dist: %d\tNew RM POS: %u\n", min_ed, pos);
		return true;
	}

	return false;
}

// pos is exclusive
// [ pos-len, pos-1 ]
bool extend_left(const vector <uint32_t>& common_tid, char* seq, uint32_t& pos, int len, int ed_th, uint32_t lb, 
					AlignRes& best_alignment) {
	int seq_len = len;
	int ref_len = len + INDELTH;
	uint32_t orig_pos = pos;

	char ref_seq[maxReadLength + 4 * INDELTH];
	ref_seq[ref_len] = '\0';
	
	int indel;
	bool consecutive = false;
	AlignRes curr_alignment(lb);
	best_alignment.set(pos, ed_th + 1, maxSc + 1, INDELTH + 1, 0);

	map <AllCoord, AlignRes> align_res;

	for (int i = 0; i < common_tid.size(); i++) {
		extend_left_trans(common_tid[i], pos, ref_seq, ref_len, seq, seq_len, ed_th, lb, best_alignment, consecutive, align_res);
		//best_alignment.print();
		// if (best_alignment.qcovlen >= seq_len and best_alignment.ed == 0 and best_alignment.sclen == 0) {
		// 	pos = best_alignment.pos + best_alignment.sclen;
		// 	return true;
		// }
	}
		
	uint32_t lmpos_best = best_alignment.pos;
	int min_ed = best_alignment.ed;
	int sclen_best = best_alignment.sclen;

	//vafprintf(2, stderr, "Min Edit Dist: %d\tNew LM POS: %u\tCovered len: %d\n", min_ed, lmpos_best, best_alignment.rcovlen);

	if (min_ed <= ed_th) {
		pos = lmpos_best + sclen_best;
		vafprintf(2, stderr, "Min Edit Dist: %d\tNew LM POS: %u\n", min_ed, pos);
		if (best_alignment.qcovlen >= seq_len)
			return true;
	}

	if (consecutive)
		return false;

	// intron retention
	if (!pac2char(orig_pos - ref_len, ref_len, ref_seq))
		return false;

	min_ed = alignment.local_alignment_left_sc(ref_seq, ref_len, seq, seq_len, sclen_best, indel);
	
	vafprintf(2, stderr, "Intron Retention:\nlmpos: %lu\textend len: %d\n", orig_pos, len);
	vafprintf(2, stderr, "str beg str:  %s\nread beg str: %s\nedit dist %d\n", ref_seq, seq, min_ed);

	if (min_ed <= ed_th) {
		curr_alignment.set(orig_pos - seq_len + indel, min_ed, sclen_best, indel, seq_len);
		best_alignment.update_left(curr_alignment);
		pos = orig_pos - seq_len + indel + sclen_best;
		vafprintf(2, stderr, "Min Edit Dist: %d\tNew LM POS: %u\n", min_ed, pos);
		return true;
	}

	return false;
}

// returns true iff extension was successful
bool extend_right_middle(uint32_t pos, char* ref_seq, uint32_t exon_len, char* qseq, int qseq_len, 
							int ed_th, AlignRes& best, AlignRes& curr, AlignRes& exon_res) {

	vafprintf(2, stderr, "Middle Right Ext Going for %lu - %lu\n", pos + 1, pos + exon_len);
	if (!pac2char(pos + 1, exon_len, ref_seq))
		return false;

	int indel;
	int seq_remain = minM(exon_len + INDELTH, qseq_len);
	int edit_dist = alignment.local_alignment_right(qseq, seq_remain, ref_seq, exon_len, indel);

	uint32_t new_rmpos = pos + exon_len - indel;
	exon_res.set(new_rmpos, edit_dist, 0, indel, exon_len);

	vafprintf(2, stderr, "rmpos: %lu\textend len: %d\tindel: %d\tedit dist: %d\n", 
							new_rmpos, exon_len, indel, edit_dist);
	vafprintf(2, stderr, "str beg str:  %s\nread beg str: %s\n", ref_seq, qseq);

	if (curr.ed + edit_dist <= ed_th) {
		curr.update(edit_dist, 0, new_rmpos, indel, exon_len);
		best.update_right(curr);
		return true;
	}
	return false;
}

void extend_right_end(uint32_t pos, char* ref_seq, uint32_t ref_len, char* qseq, int qseq_len, 
						int ed_th, AlignRes& best, AlignRes& curr, AlignRes& exon_res) {

	vafprintf(2, stderr, "Final Right Ext Going for %lu - %lu\n", pos + 1, pos + ref_len);
	if (!pac2char(pos + 1, ref_len, ref_seq))
		return;

	int sclen, indel;
	int edit_dist = alignment.local_alignment_right_sc(ref_seq, ref_len, qseq, qseq_len, sclen, indel);

	uint32_t new_rmpos = pos + qseq_len - indel;
	exon_res.set(new_rmpos, edit_dist, sclen, indel, qseq_len);
	
	vafprintf(2, stderr, "rmpos: %lu\textend len: %d\tindel: %d\tedit dist: %d\tsclen: %d\n", 
							new_rmpos, qseq_len, indel, edit_dist, sclen);
	vafprintf(2, stderr, "str beg str:  %s\nread beg str: %s\n", ref_seq, qseq);
	
	if (curr.ed + edit_dist <= ed_th) {
		curr.update(edit_dist, sclen, new_rmpos, indel, qseq_len);
		best.update_right(curr);
	}
}

// [pos + 1, pos + len]
void extend_right_trans(uint32_t tid, uint32_t pos, char* ref_seq, int ref_len, char* qseq, int qseq_len, 
						int ed_th, uint32_t ub, AlignRes& best, bool& consecutive, map <AllCoord, AlignRes>& align_res) {
	consecutive = false;
	AlignRes curr(ub);
	AlignRes exon_res(ub);

	int it_ind;
	const IntervalInfo<UniqSeg>* it_seg = gtf_parser.get_location_overlap_ind(pos, false, it_ind);
	if (it_seg == NULL) {	// probably wrong chaining to intron 
		return;
		it_seg = gtf_parser.get_interval(it_ind);
		int diff = pos - it_seg->epos;
		qseq -= diff;
		qseq_len += diff;
		ref_len += diff;
		pos = gtf_parser.get_interval(it_ind + 1)->spos - 1;
	}
	
	int it_ind_start = gtf_parser.get_trans_start_ind(contigNum, tid);
	int rel_ind = it_ind - it_ind_start;
	int curr_exon_start_ind = it_ind;
	int curr_exon_end_ind = it_ind;
	
	uint32_t rspos = pos;
	int exon_len = it_seg->epos - pos;
	int remain_ref_len = ref_len;
	int covered = 0;
	int indel;

	for (int i = rel_ind + 1; i < gtf_parser.trans2seg[contigNum][tid].size(); i++) {
		if (exon_len >= qseq_len - covered)
			break;
		if (gtf_parser.trans2seg[contigNum][tid][i] == 1) {
			// go for alignment
			indel = 0;
			if (exon_len > 0) {
				if (rspos + exon_len > ub) {
					return;
				}

				int remain_qseq_len = minM(exon_len + INDELTH, qseq_len - covered);

				// search in map
				AllCoord tmp_coord(rspos, exon_len, covered, remain_qseq_len);
				map <AllCoord, AlignRes>::iterator it;
				it = align_res.find(tmp_coord);
				if (it != align_res.end()) {
					vafprintf(2, stderr, "[Found] Middle Right Ext Going for %lu - %lu\n", rspos + 1, rspos + exon_len);
					vafprintf(2, stderr, "rmpos: %lu\textend len: %d\tindel: %d\tedit dist: %d\n", 
							it->second.pos, it->second.qcovlen, it->second.indel, it->second.ed);

					if (curr.ed + it->second.ed > ed_th)
						return;
					else {
						curr.update(it->second.ed, it->second.sclen, it->second.pos, it->second.indel, it->second.qcovlen);
						best.update_right(curr);
					}

					indel = it->second.indel;
				}
				else {
					bool success = extend_right_middle(rspos, ref_seq, exon_len, qseq + covered, remain_qseq_len, 
													ed_th, best, curr, exon_res);

					align_res.insert(pair <AllCoord, AlignRes>(tmp_coord, exon_res));
					if (! success)
						return;

					indel = exon_res.indel;
				}
			}
			//

			remain_ref_len -= exon_len;
			covered += exon_len - indel;
			exon_len = 0;
			curr_exon_start_ind = i + it_ind_start;
			curr_exon_end_ind = i + it_ind_start;
			it_seg = gtf_parser.get_interval(i + it_ind_start);
			rspos = it_seg->spos - 1;
		}
		if (gtf_parser.trans2seg[contigNum][tid][i] != 0) {
			curr_exon_end_ind = i + it_ind_start;
			it_seg = gtf_parser.get_interval(curr_exon_end_ind);
			exon_len += it_seg->epos - it_seg->spos + 1;
		}
	}

	if (covered >= qseq_len or (rspos + qseq_len - covered > ub) or (exon_len < qseq_len - covered))	// last argument is for when we reach end of transcript and read remains
		return;

	consecutive = (rspos == pos);

	remain_ref_len = minM(remain_ref_len, exon_len);
	// search in map
	AllCoord tmp_coord(rspos, remain_ref_len, covered, qseq_len - covered);
	map <AllCoord, AlignRes>::iterator it;
	it = align_res.find(tmp_coord);
	if (it != align_res.end()) {
		vafprintf(2, stderr, "[Found] Final Right Ext Going for %lu - %lu\n", rspos + 1, rspos + remain_ref_len);
		vafprintf(2, stderr, "rmpos: %lu\textend len: %d\tindel: %d\tedit dist: %d\tsclen: %d\n", 
							it->second.pos, it->second.qcovlen, it->second.indel, it->second.ed, it->second.sclen);

		if (curr.ed + it->second.ed > ed_th)
			return;
		else {
			curr.update(it->second.ed, it->second.sclen, it->second.pos, it->second.indel, it->second.qcovlen);
			best.update_right(curr);
		}
	}
	else {
		extend_right_end(rspos, ref_seq, remain_ref_len, qseq + covered, qseq_len - covered, ed_th, best, curr, exon_res);
		align_res.insert(pair <AllCoord, AlignRes>(tmp_coord, exon_res));
	}
}

// returns true iff extension was successful
bool extend_left_middle(uint32_t pos, char* ref_seq, uint32_t exon_len, char* qseq, int qseq_len, 
							int ed_th, AlignRes& best, AlignRes& curr, AlignRes& exon_res) {

	vafprintf(2, stderr, "Middle Left Ext Going for %lu - %lu\n", pos - exon_len, pos - 1);
	if (!pac2char(pos - exon_len, exon_len, ref_seq))
		return false;

	int indel;
	int seq_remain = minM(exon_len + INDELTH, qseq_len);
	int edit_dist = alignment.local_alignment_left(qseq, seq_remain, ref_seq, exon_len, indel);

	uint32_t new_lmpos = pos - exon_len + indel;
	exon_res.set(new_lmpos, edit_dist, 0, indel, exon_len);

	vafprintf(2, stderr, "lmpos: %lu\textend len: %d\tindel: %d\tedit dist: %d\n", 
							new_lmpos, exon_len, indel, edit_dist);
	vafprintf(2, stderr, "str beg str:  %s\nread beg str: %s\n", ref_seq, qseq);

	if (curr.ed + edit_dist <= ed_th) {
		curr.update(edit_dist, 0, new_lmpos, indel, exon_len);
		best.update_left(curr);
		return true;
	}
	return false;
}

void extend_left_end(uint32_t pos, char* ref_seq, uint32_t ref_len, char* qseq, int qseq_len, 
						int ed_th, AlignRes& best, AlignRes& curr, AlignRes& exon_res) {

	vafprintf(2, stderr, "Final Left Ext Going for %lu - %lu\n", pos - ref_len, pos - 1);
	if (!pac2char(pos - ref_len, ref_len, ref_seq))
		return;

	int sclen, indel;
	int edit_dist = alignment.local_alignment_left_sc(ref_seq, ref_len, qseq, qseq_len, sclen, indel);

	uint32_t new_lmpos = pos - qseq_len + indel;
	exon_res.set(new_lmpos, edit_dist, sclen, indel, qseq_len);
	
	vafprintf(2, stderr, "lmpos: %lu\textend len: %d\tindel: %d\tedit dist: %d\tsclen: %d\n", 
							new_lmpos, qseq_len, indel, edit_dist, sclen);
	vafprintf(2, stderr, "str beg str:  %s\nread beg str: %s\n", ref_seq, qseq);
	
	if (curr.ed + edit_dist <= ed_th) {
		curr.update(edit_dist, sclen, new_lmpos, indel, qseq_len);
		best.update_left(curr);
	}
}

// [pos - len, pos - 1]
void extend_left_trans (uint32_t tid, uint32_t pos, char* ref_seq, int ref_len, char* qseq, int qseq_len, 
						int ed_th, uint32_t lb,  AlignRes& best, bool& consecutive, map <AllCoord, AlignRes>& align_res) {

	consecutive = false;
	AlignRes curr(lb);
	AlignRes exon_res(lb);

	int it_ind;
	const IntervalInfo<UniqSeg>* it_seg = gtf_parser.get_location_overlap_ind(pos, false, it_ind);
	if (it_seg == NULL) {	// probably wrong chaining to intron 
		return;
		it_seg = gtf_parser.get_interval(it_ind + 1);
		int diff = it_seg->spos - pos;
		qseq_len += diff;
		ref_len += diff;
		pos = it_seg->spos + 1;
	}

	int it_ind_start = gtf_parser.get_trans_start_ind(contigNum, tid);
	int rel_ind = it_ind - it_ind_start;
	int curr_exon_start_ind = it_ind;
	int curr_exon_end_ind = it_ind;
	
	uint32_t lepos = pos;
	int exon_len = 0;
	int remain_ref_len = ref_len;
	int covered = 0;
	int indel;
	bool first_seg = true;

	for (int i = rel_ind; i >= 0; i--) {
		if (gtf_parser.trans2seg[contigNum][tid][i] != 0) {
			curr_exon_start_ind = i + it_ind_start;
			it_seg = gtf_parser.get_interval(curr_exon_start_ind);
			if (first_seg) {
				exon_len = pos - it_seg->spos;
				first_seg = false;
			}
			else {
				if (exon_len == 0) {
					curr_exon_start_ind = i + it_ind_start;
					curr_exon_end_ind = i + it_ind_start;
					lepos = it_seg->epos + 1;
				}
				exon_len += it_seg->epos - it_seg->spos + 1;
			}
		}

		if (exon_len >= qseq_len - covered)
			break;

		if (gtf_parser.trans2seg[contigNum][tid][i] == 1) {
			// go for alignment
			indel = 0;
			if (exon_len > 0) {
				if (lepos < lb + exon_len) {
					return;
				}

				int remain_qseq_len = minM(exon_len + INDELTH, qseq_len - covered);

				// search in map
				AllCoord tmp_coord(lepos, exon_len, covered, remain_qseq_len);
				map <AllCoord, AlignRes>::iterator it;
				it = align_res.find(tmp_coord);
				if (it != align_res.end()) {
					vafprintf(2, stderr, "[Found] Middle Left Ext Going for %lu - %lu\n", lepos - exon_len, lepos - 1);
					vafprintf(2, stderr, "lmpos: %lu\textend len: %d\tindel: %d\tedit dist: %d\n", 
							it->second.pos, it->second.qcovlen, it->second.indel, it->second.ed);

					if (curr.ed + it->second.ed > ed_th)
						return;
					else {
						curr.update(it->second.ed, it->second.sclen, it->second.pos, it->second.indel, it->second.qcovlen);
						best.update_left(curr);
					}

					indel = it->second.indel;
				}
				else {
					bool success = extend_left_middle(lepos, ref_seq, exon_len, qseq + qseq_len - covered - remain_qseq_len, remain_qseq_len, 
													ed_th, best, curr, exon_res);

					align_res.insert(pair <AllCoord, AlignRes>(tmp_coord, exon_res));
					if (! success)
						return;

					indel = exon_res.indel;
				}
			}
			//

			remain_ref_len -= exon_len;
			covered += exon_len - indel;
			exon_len = 0;
		}
	}

	if (covered >= qseq_len or (lepos < lb + qseq_len - covered) or (exon_len < qseq_len - covered))
		return;

	consecutive = (lepos == pos);

	remain_ref_len = minM(remain_ref_len, exon_len);
	// search in map
	AllCoord tmp_coord(lepos, remain_ref_len, covered, qseq_len - covered);
	map <AllCoord, AlignRes>::iterator it;
	it = align_res.find(tmp_coord);
	if (it != align_res.end()) {
		vafprintf(2, stderr, "[Found] Final Left Ext Going for %lu - %lu\n", lepos - remain_ref_len, lepos - 1);
		vafprintf(2, stderr, "lmpos: %lu\textend len: %d\tindel: %d\tedit dist: %d\tsclen: %d\n", 
							it->second.pos, it->second.qcovlen, it->second.indel, it->second.ed, it->second.sclen);

		if (curr.ed + it->second.ed > ed_th)
			return;
		else {
			curr.update(it->second.ed, it->second.sclen, it->second.pos, it->second.indel, it->second.qcovlen);
			best.update_left(curr);
		}
	}
	else {
		extend_left_end(lepos, ref_seq, remain_ref_len, qseq, qseq_len - covered, ed_th, best, curr, exon_res);
		align_res.insert(pair <AllCoord, AlignRes>(tmp_coord, exon_res));
	}
}
