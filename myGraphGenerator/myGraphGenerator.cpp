#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphml.hpp>
#include <boost/graph/random.hpp>
#include <boost/random.hpp>
#include <boost/program_options.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <unordered_set>
#include <vector>
#include <ctime>
#include <cstdint>
#include <ostream>
#include <string>
#include <cmath>

using namespace boost;
namespace po = boost::program_options;

typedef dynamic_bitset<> db;

typedef typename adjacency_list<vecS, vecS, undirectedS, no_property, property<edge_color_t, int>> Graph;


template <typename EdgeColorMap, typename ValidColorsMap>
struct valid_edge_color {
	valid_edge_color() { }
	valid_edge_color(EdgeColorMap color, ValidColorsMap v_colors) : m_color(color), v_map(v_colors) { }
	template <typename Edge>
	bool operator()(const Edge& e) const {
		return v_map.test(get(m_color, e));
	}
	EdgeColorMap m_color;
	ValidColorsMap v_map;
};

template<class Graph, class Mask>
int get_components(Graph &g, Mask &m, std::vector<int> &components) {
	typedef typename property_map<Graph, edge_color_t>::type EdgeColorMap;
	typedef typename boost::dynamic_bitset<> db;
	typedef filtered_graph<Graph, valid_edge_color<EdgeColorMap, db> > fg;

	valid_edge_color<EdgeColorMap, Mask> filter(get(edge_color, g), m);
	fg tg(g, filter);
	int num = connected_components(tg, &components[0]);
	return num;
}

template <class Graph>
int kLSFMVCA(Graph &g,int k_sup,int n_labels)	{
	std::vector<int> components(num_vertices(g));
		db temp(n_labels);
		int num_c = get_components(g, temp, components);
		int num_c_best = num_c;
		while (num_c_best > 1 && temp.count() < k_sup) {
		    int best_label = 0;
			for (int i = 0; i < n_labels; ++i) {
				if (!temp.test(i)) {
					temp.set(i);
					int nc = get_components(g, temp, components);
					if (nc <= num_c_best) {
						num_c_best = nc;
						best_label = i;
					}
					temp.flip(i);
				}
			}
			if (temp.count() == n_labels)break;
			temp.set(best_label);
		}
		return num_c_best;
}
void generate_graph(const std::vector<int>& v_size, const std::vector<double>& l_mul,int n_iter) {
	std::time_t now = std::time(0);
	random::mt19937 gen{ static_cast<std::uint32_t>(now) };

	int size = v_size.size();
	int size_l = l_mul.size();
	for (int i = 0; i < size; ++i) {
		int l, v, e;
		v = v_size[i];
		e = (v - 1)*v / 10;
		for (int k = 0; k < size_l; ++k) {
			for (int j = 1; j <= n_iter; ++j) {
				Graph g;
				l = v * l_mul[k];
				random::uniform_int_distribution<> dist{ 0, l - 1 };
				variate_generator<random::mt19937&, random::uniform_int_distribution<>> rand_gen(gen, dist);
				int J, k_sup;
				J = 0;
				generate_random_graph(g, v, e, gen, false, false);
				boost::randomize_property<edge_color_t>(g, rand_gen);
				//to fix to force l random colors.
				for (int i = 0; i < l; ++i) {
					auto it = random_edge(g, gen);
					auto color = get(edge_color, g);
					//std::cout << it<< color[it] <<std::endl;
					color[it] = i;
					//std::cout << it << color[it] << std::endl;
				}
				do {
					k_sup = v / std::pow(2, J++);
				} while (kLSFMVCA(g, k_sup, l) == 1);
				k_sup = v / std::pow(2, --J);
				auto colors = get(edge_color, g);
				dynamic_properties dp;
				dp.property("color", colors);
				std::ofstream tmp(std::to_string(v) + "-" + std::to_string(e) + "-" + std::to_string(l) + "-"
					+ std::to_string(k_sup) + "-" + std::to_string(j) + ".graphml");
				write_graphml(tmp, g, dp, true);
			}
		}
	}
}
int main(int argc, const char *argv[])
{
	int n_vertices, n_colors;
	//command-line processor

	try {
		std::ifstream ifn;
		po::options_description desc{ "Options" };
		desc.add_options()("help,h", "produce help message")
			("input-file,i", po::value< std::vector<std::string> >()->multitoken(), "input file(unique format -f to see format)")
			("include-path,I", po::value< std::string >(), "include path")
			("number-vertices,n", po::value< std::vector<int> >()->multitoken(),"number of vertices")
			("multipliers,m", po::value<std::vector<double> >()->multitoken(), "multipliers")
			("iter,j", po::value<int >(), "iterations")
			("help-format,f","input format description");
		po::positional_options_description p;
		p.add("input-file", -1);


		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			return 1;
		}
		else if (vm.count("input-file"))
		{
			std::vector<std::string> inputs = vm["input-file"].as<std::vector<std::string>>();
			if (vm.count("include-path")) std::cout << "Include Path is " << vm["include-path"].as<std::string>() << "\n";
			for (auto it = inputs.begin(); it != inputs.end(); ++it) {
				std::cout << "Input file is " << *it << "\n";
				if (vm.count("include-path")) {
					ifn.open((vm["include-path"].as<std::string>() + *it).c_str(), std::ifstream::in);
				}
				else ifn.open((*it).c_str(), std::ifstream::in);
				if (!ifn.is_open()) {
					std::cout << "error opening file" << std::endl;
					exit(EXIT_FAILURE);
				}
				int n, l,k_sup;
				int j = 1;
				Graph g;
				ifn >> n >> l;
				unordered_set<int> mset;
				do{
					int color;
					k_sup = 0;
					g.clear();
					for (int u = 0; u < n; u++) {
						for (int v = u + 1; v < n; v++) {
							ifn >> color;
							//std::cout << color << std::endl;
							mset.insert(color);
							if (color < l) {
								boost::add_edge(u, v, edge_color_t(color), g);
							}
						}
					}
					int J = 0;
					int nc;
					do {
						k_sup = n / std::pow(2, J++);
						nc = kLSFMVCA(g, k_sup, l);
						std::cout << nc <<" "<< k_sup << std::endl;
					} while (nc == 1);
					k_sup = n / std::pow(2, --J);
					auto colors = get(edge_color, g);
					dynamic_properties dp;
					dp.property("color", colors);
					int e = num_edges(g);
					std::ofstream tmp(std::to_string(n) + "-" + std::to_string(e) + "-" + std::to_string(l) + "-"
						+ std::to_string(k_sup) + "-" + std::to_string(j++) + ".graphml");
					write_graphml(tmp, g, dp, true);
					std::cout << mset.size()<<std::endl;
					mset.clear();
				} while (!ifn.eof());
				ifn.close();
			}
			
		}
		else if (vm.count("number-vertices") && vm.count("multipliers") && vm.count("iter")) {
			generate_graph(vm["number-vertices"].as<std::vector<int>>(),
				vm["multipliers"].as<std::vector<double>>(),
				vm["iter"].as<int>());
		}
		else {
			std::cout << "see options(-h)." << std::endl;
		}


	}
	catch (const po::error &ex) {
		std::cout << ex.what();
		exit(EXIT_FAILURE);
	}
	catch (boost::exception &ex) {
		std::cout << boost::diagnostic_information(ex) << std::endl;
	}
	catch (std::exception &ex) {
		std::cout << ex.what();
		exit(EXIT_FAILURE);
	}
	
	return 0;
}
