#include <algorithm>
#include <iostream>
#include <fstream>
//#include <unordered_map>
#include <cstdio>
#include <string>
#include <map>
#include <zlib.h>
#include <getopt.h>
//#include "partition.h"
#include "common.h"
#include "fasta.h"
#include "geneannotation.h"
//#include "assembler_old.h"
//#include "assembler_ext.h"
//#include "kmistrvar.h"
//#include "genome.h"
//#include "simulator.h"
//#include "common.h"
//#include "record.h"
//#include "sam_parser.h"
//#include "bam_parser.h"
//#include "extractor.h"
//#include "sort.h"

using namespace std;

char versionNumber[10]="1.0.0";

/********************************************************************/
inline string space (int i) 
{
	return string(i, ' ');
}

/********************************************************************/
inline string itoa (int i)
{
	char c[50];
	sprintf(c, "%d", i);
	return string(c);
}

///********************************************************************/
//void partify (const string &read_file, const string &mate_file, const string &out, int threshold) 
//{
//	gzFile gzf = gzopen(mate_file.c_str(), "rb");
//	unordered_map<string, string> mymap;
//
//	const int MAXB = 8096;
//	char buffer[MAXB];
//	char name[MAXB], read[MAXB];
//	while (gzgets(gzf, buffer, MAXB)) {
//		sscanf(buffer, "%s %s", name, read);
//		string read_name = string(name+1); 
//		if (read_name.size() > 2 && read_name[read_name.size() - 2] == '/')
//			read_name = read_name.substr(0, read_name.size() - 2);
//		mymap[read_name] = string(read);
//	}
//
//	genome_partition pt(read_file, threshold, mymap); 
//
//	FILE *fo = fopen(out.c_str(), "wb");
//	FILE *fidx = fopen((out + ".idx").c_str(), "wb");
//	while (pt.has_next()) {
//		auto p = pt.get_next();
//		size_t i = pt.dump(p, fo);
//		fwrite(&i, 1, sizeof(size_t), fidx);
//	}
//	fclose(fo);
//	fclose(fidx);
//	printf("%d\n", pt.get_cluster_id());
//}
//
///********************************************************************/
//void predict (const string &partition_file, const string &reference, const string &gtf, const bool barcodes, const bool print_reads, const bool print_stats, const string &range, const string &out_vcf,
//					int k, int anchor_len, int min_support, int max_support, int uncertainty, int min_length, int max_length, int max_reads, const bool LOCAL_MODE, int ref_flank)
//{
//	kmistrvar predictor(k, anchor_len, partition_file, reference, gtf, barcodes, print_reads, print_stats, max_reads );
//	predictor.run_kmistrvar(range, out_vcf, min_support, max_support, uncertainty, min_length, max_length, LOCAL_MODE, ref_flank);
//}
//
///********************************************************************/
//bool isOEA (uint32_t flag)
//{
//	return (/*!flag ||*/ ((flag & 0x8) && (flag & 0x4)) 
//				      || ((flag & 0x8) && !(flag & 0x4)) 
//				      || (!(flag & 0x8) && (flag & 0x4)));
//}
//
///********************************************************************/
//bool isOEA_mrsFAST (uint32_t flag)
//{
//	return (/*!flag ||*/ ((flag & 0x8) && (flag & 0x4)) 
//				      || ((flag & 0x8) && !(flag & 0x4)) 
//				      || (!(flag & 0x8) && (flag & 0x4)));
//}
//
///********************************************************************/
//void extractOEA (const string &path, const string &result, bool fasta = false) 
//{
//	FILE *fi = fopen(path.c_str(), "rb");
//	char mc[2];
//	fread(mc, 1, 2, fi);
//	fclose(fi);
//
//	Parser *parser;
//	if (mc[0] == char(0x1f) && mc[1] == char(0x8b)) 
//		parser = new BAMParser(path);
//	else
//		parser = new SAMParser(path);
//
//	string comment = parser->readComment();
//
//	gzFile output = gzopen(result.c_str(), "wb6");
//	//if (!fasta)
//	//	gzwrite(output, comment.c_str(), comment.size());
//	while (parser->hasNext()) {
//		const Record &rc = parser->next();
//		if (isOEA(rc.getMappingFlag())) {
//			string record;
//			if (fasta) 
//				record = S("@%s\n%s\n+\n%s\n", rc.getReadName(), rc.getSequence(), rc.getQuality());
//			else {
//				char *readName = (char*)rc.getReadName();
//				int l = strlen(readName);
//				if (l > 1 && readName[l - 2] == '/')
//					readName[l - 2] = '\0';
//				record = S("%s %d %s %d %d %s %s %d %d %s %s %s\n",
//            		readName,
//            		rc.getMappingFlag(),
//            		rc.getChromosome(),
//            		rc.getLocation(),
//            		rc.getMappingQuality(),
//            		rc.getCigar(),
//            		rc.getPairChromosome(),
//            		rc.getPairLocation(),
//            		rc.getTemplateLength(),
//            		rc.getSequence(),
//            		rc.getQuality(),
//            		rc.getOptional()
//        		);
//			}
//			gzwrite(output, record.c_str(), record.size());
//		}
//		parser->readNext();
//	}
//
//	gzclose(output);
//	delete parser;
//}
//
///********************************************************************/
//void mask (const string &repeats, const string &path, const string &result, int pad = 0, bool invert = false)
//{
//	const int MAX_BUF_SIZE = 2000;
//	size_t b, e, m = 0;
//	char ref[MAX_BUF_SIZE], name[MAX_BUF_SIZE], l[MAX_BUF_SIZE];
//	
//	map<string, vector<pair<size_t, size_t>>> masks;
//	FILE *fi = fopen(repeats.c_str(), "r");
//	int prev = 0; string prev_ref = "";
//	while (fscanf(fi, "%s %lu %lu %s", ref, &b, &e, name) != EOF) {
//		if (e - b < 2 * pad) continue;
//		masks[ref].push_back({b + pad, e - pad});
//	}
//	for (auto &r: masks) 
//		sort(r.second.begin(), r.second.end());
//	if (invert) for (auto &r: masks) 
//	{
//		vector<pair<size_t, size_t>> v;
//		size_t prev = 0;
//		for (auto &p: r.second) {
//			if (prev < p.first)
//				v.push_back({prev, p.first});
//			prev = p.second;
//		}
//		v.push_back({prev, 500 * 1024 * 1024});
//		r.second = v;
//	}
//	fclose(fi);
//	
//	const int MAX_CHR_SIZE = 300000000;
//	char *x = new char[MAX_CHR_SIZE];
//
//	fi = fopen(path.c_str(), "r");
//	FILE *fo = fopen(result.c_str(), "w");
//	fgets(l, MAX_BUF_SIZE, fi);
//	
//	while (true) {
//		if (feof(fi))
//			break;
//		
//		fprintf(fo, "%s", l);
//		char *p = l; while (*p && !isspace(*p)) p++; *p = 0;
//		string n = string(l + 1);
//		printf("Parsed %s\n", n.c_str());
//		
//		size_t xlen = 0;
//		while (fgets(l, MAX_BUF_SIZE, fi)) {
//			if (l[0] == '>') 
//				break;
//			memcpy(x + xlen, l, strlen(l) - 1);
//			xlen += strlen(l) - 1;
//		}
//		x[xlen] = 0;
//
//		for (auto &r: masks[n]) {
//			if (r.first >= xlen) continue;
//			if (r.second >= xlen) r.second = xlen;
//			memset(x + r.first, 'N', r.second - r.first);
//		}
//
//		for (size_t i = 0; i < xlen; i += 120) 
//			fprintf(fo, "%.120s\n", x + i);
//	}
//
//	fclose(fi);
//	fclose(fo);
//	delete[] x;
//}
//
///********************************************************************/
//void removeUnmapped (const string &path, const string &result)
//{
//	ifstream fin(path.c_str());
//	FILE *fo = fopen(result.c_str(), "w");
//	
//	string l, a, b, aa, bb; 
//	while (getline(fin, a)) {
//		getline(fin, b);
//		getline(fin, l);
//		getline(fin, l);
//
//		int ll = a.size();
//		if (ll > 1 && a[ll - 2] == '/')
//			a = a.substr(0, ll - 2);
//		if (a == aa)
//			continue;
//		else {
//			fprintf(fo, "%s %s\n", aa.c_str(), bb.c_str());
//			aa = a, bb = b;
//		}
//	}
//	fprintf(fo, "%s %s\n", aa.c_str(), bb.c_str());
//
//	fin.close();
//	fclose(fo);
//}
//
///********************************************************************/
//void sortSAM (const string &path, const string &result) 
//{
//	sortFile(path, result, 2 * GB);
//}
//
///********************************************************************/
//void copy_string_reverse( char *src, char *dest)
//{
//	int limit=strlen(src);
//	for( int i = 0; i < limit; i ++)
//	{
//		dest[i]=src[limit-1-i];
//	}
//	dest[limit]='\0';
//}
//
///**********************************************/
//void copy_string_rc( char *src, char *dest)
//{
//	int limit=strlen(src);
//	for( int i = 0; i < limit; i ++)
//	{
//		switch( src[limit-1-i])
//		{
//			case 'A':
//				dest[i]='T';
//				break;
//			case 'C':
//				dest[i]='G';
//				break;
//			case 'G':
//				dest[i]='C';
//				break;
//			case 'T':
//				dest[i]='A';
//				break;
//			default:
//				dest[i]='N';
//				break;
//		}
//	}
//	dest[limit]='\0';
//}
//
///********************************************************************/
//void extractMrsFASTOEA (const string &sam, const string &out) {
//	string s;
//	ifstream fin(sam.c_str());
//	char name1[300], seq1[300], qual1[300];
//	char name2[300], seq2[300], qual2[300];
//	char seq1_new[300], qual1_new[300];
//	char seq2_new[300], qual2_new[300];
//	int flag1, flag2;
//	FILE *fm = fopen(string(out + ".mapped.fq").c_str(), "wb");
//	FILE *fu = fopen(string(out + ".unmapd.fq").c_str(), "wb");
//
//	while (getline(fin, s)) {
//		if (s[0] == '@') continue;
//		sscanf(s.c_str(), "%s %d %*s %*d %*s %*s %*s %*s %*s %s %s",
//				name1, &flag1, seq1, qual1);
//
//		getline(fin, s);
//		sscanf(s.c_str(), "%s %d %*s %*d %*s %*s %*s %*s %*s %s %s",
//				name2, &flag2, seq2, qual2);
//
//		if (flag1 == 0 && flag2 == 4)  {
//			if (checkNs(seq2)) wo(fm, name1, seq1, qual1), wo(fu, name2, seq2, qual2);
//		}
//		if (flag1 == 16 && flag2 == 4) {
//			copy_string_reverse(qual1, qual1_new);
//			copy_string_rc(seq1, seq1_new);
//			if (checkNs(seq2)) wo(fm, name1, seq1_new, qual1_new), wo(fu, name2, seq2, qual2);
//		}
//		if (flag1 == 4 && flag2 == 0)  {
//			if (checkNs(seq1)) wo(fu, name1, seq1, qual1), wo(fm, name2, seq2, qual2);
//		}
//		if (flag1 == 4 && flag2 == 16) {
//			copy_string_reverse(qual2, qual2_new);
//			copy_string_rc(seq2, seq2_new);
//			if (checkNs(seq1)) wo(fu, name1, seq1, qual1), wo(fm, name2, seq2_new, qual2_new);
//		}
//	}
//	fclose(fm), fclose(fu);
//	fin.close();
//}
//
///********************************************************************/
//void simulate_SVs (string in_file, string ref_file, bool from_bed) {
//
//	simulator sim(ref_file);
//
//	if(from_bed){
//		sim.simulate(in_file);
//	}
//	else{
//		sim.simulate_from_file(in_file);
//	}
//	
//}
//
///********************************************************************/
//void annotate (string gtf, string in_file, string out_file, bool genomic) {
//
//	string line, chr, best_gene_s, best_name_s, best_trans_s, best_gene_e, best_name_e, best_trans_e, context_s, context_e, key, gene_id, trans_id;
//	int start, end, t_start, t_end;
//	ifstream infile (in_file);
//	ofstream outfile (out_file);
//	vector<uint32_t> vec_best_s, vec_best_e;
//	vector<string> tokens;
//	map <string,vector<isoform>> iso_gene_map;
//	map <string,vector<gene_data>> gene_sorted_map;
//	map <string,vector<uint32_t>> vec_all_s, vec_all_e;
//	vector<string> contexts ={"intergenic", "intronic", "non-coding", "UTR", "exonic-CDS"};
//	ensembl_Reader(gtf.c_str(), iso_gene_map, gene_sorted_map );
//
//	if (infile.is_open()){
//		while ( getline (infile,line)){
//
//			tokens = split(line, '\t'); 
//			chr = tokens[0];
//			start = atoi(tokens[1].c_str());
//			end = atoi(tokens[2].c_str());
//
//			if(genomic){
//				locate_interval(chr, start, start, gene_sorted_map[chr], 0, iso_gene_map, best_gene_s, best_name_s, best_trans_s, vec_best_s, vec_all_s);
//				locate_interval(chr, end, end, gene_sorted_map[chr], 0, iso_gene_map, best_gene_e,  best_name_e, best_trans_e, vec_best_e, vec_all_e);
//			}
//			else{
//				gene_id = tokens[3];
//				trans_id = tokens[4];
//
//				locate_interval(chr, start, start, gene_id, trans_id, gene_sorted_map[chr], 0, iso_gene_map, best_gene_s, best_name_s, best_trans_s, vec_best_s, vec_all_s);
//				locate_interval(chr, end, end, gene_id, trans_id, gene_sorted_map[chr], 0, iso_gene_map, best_gene_e, best_name_e, best_trans_e, vec_best_e, vec_all_e);
//			}
//			
//			for (auto & match : vec_all_s){
//				key = match.first;
//				if(!vec_all_e[key].empty()){
//
//					t_start = match.second[1];
//					t_end = vec_all_e[key][1];
//
//					context_s = contexts[vec_best_s[0]];
//					context_e = contexts[vec_best_e[0]];
//
//					if(t_start < t_end){
//						outfile << chr << "\t" << start << "\t" << end << "\t" << key << "\t" << t_start << "\t" << t_end << "\t" << context_s << "\t" << context_e << endl;
//					}
//					else if(t_start > t_end){
//						outfile << chr << "\t" << start << "\t" << end << "\t" << key << "\t" << t_end << "\t" << t_start << "\t" << context_e << "\t" << context_s << endl;
//					}
//					else{
//						outfile << chr << "\t" << start << "\t" << end << "\t" << key << "\tmissing\tmissing\tNA\tNA" << endl;
//					}
//				}
//				else{
//					outfile << chr << "\t" << start << "\t" << end << "\t" << key << "\tmissing\tmissing\tNA\tNA" << endl;
//				}
//			}
//
//			vec_best_s.clear();
//			vec_best_e.clear();
//			vec_all_s.clear();
//			vec_all_e.clear();
//		}
//		infile.close();
//		outfile.close();
//	}
//
//	
//}
/**********************************************/
void printHELP()
{
	L( "CIRC: Transcriptome Aberration Without Alignments\n");
	L( "\t-h|--help:\tShows help message.\n");
	L( "\t-v|--version:\tShows current version.\n");
	L( "\t\nMandatory Parameters:\n");
	L( "\t-i|--input:\tInput fastq file.\n");
	L( "\t-r|--reference:\tReference Genome. Required for SV detection.\n" );
	L( "\t-g|--annotation:\tGTF file for gene annotation.\n" );
	L( "\t-o|--output:\tPrefix or output file\n");
	//LOG( "\t\nParameters for Supplementary Information:");
	//LOG( "\t-x|--unmapped:\tUnmapped reads file for generating paritions.");
	L( "\t\nOptional Parameters:\n");
	L( "\t-k|--kmer:\tKmer length (default 14).\n");
	//LOG( "\t-b|--barcode:\tInput reads contain barcodes.");
	//LOG( "\t-p|--print_reads:\tPrint all contigs and associated reads as additional output.");
	//LOG( "\t-P|--print_stats:\tPrint statistics as additional output.");
	//LOG( "\t-c|--cluster:\tClustering threshold (default 1000).");
	//LOG( "\t-k|--kmer:\tKmer length (default 14).");
	//LOG( "\t-a|--anchor:\tAnchor length (default 40).");
	//LOG( "\t-s|--min_support:\tMin Read Support (default 2).");
	//LOG( "\t-S|--max_support:\tMax Read Support (default unlimited).");
	//LOG( "\t-u|--uncertainty:\tUncertainty (default 8).");
	//LOG( "\t-m|--min_length:\tMin SV length (default 60).");
	//LOG( "\t-M|--max_length:\tMax SV length (default 20000).");
	//LOG( "\t-n|--max_reads:\tMax number of reads allowed in a cluster. \t\tA region with more than the number of OEA/clipped reads will not be considered in prediction. (default 10000).");
}
/********************************************************************/
int main(int argc, char *argv[])
{

	int opt;
	int opt_index;

	string	input_file  = "" ,
			reference  = "" ,
			out_prefix = "out" ,
			annotation = "";
	int k = 14;
	int op_code = 0;

	static struct option long_opt[] =
	{
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'v' },
		{ "input", required_argument, 0, 'i' },
		{ "reference", required_argument, 0, 'r' },
		{ "output", required_argument, 0, 'o' },
		{ "annotation", required_argument, 0, 'g' },
		{ "kmer", required_argument, 0, 'k' },
		{0,0,0,0},
	};

	while ( -1 !=  (opt = getopt_long( argc, argv, "hvi:r:o:g:k:", long_opt, &opt_index )  ) )
	{
		switch(opt)
		{
			case 'h':
				printHELP();
				return 0;
			case 'v':
				fprintf(stdout, "%s\n", versionNumber );
				return 0;
			case 'i':
				input_file.assign( optarg );
				//sam_flag = 1;
				break;
			case 'r':
				reference.assign( optarg );
				//ref_flag = 1;
				break;
			case 'o':
				out_prefix.assign( optarg );
				break;
			case 'g':
				annotation.assign( optarg );
				break;
			case 'k':
				k = atoi(optarg);
				break;
			case '?':
				fprintf(stderr, "Unknown parameter: %s\n", long_opt[opt_index].name);
				return 1;
			default:
				printHELP();
				return 0;
		}
	}

	map<string, string> genome;

	if ( 0 == op_code  )
	{
		//E("Reading Genome Information from %s\n", reference.c_str() );
		//LoadFasta( reference.c_str(), genome );
		E("Reading Gene Annotation %s\n", annotation.c_str() );
		ensembl_gtf_reader( annotation.c_str() );
	}	
	return 0;
}
