#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphml.hpp>
#include <boost/graph/random.hpp>
#include <boost/random.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/dynamic_bitset.hpp>
#include <vector>
#include <ctime>
#include <cstdint>
#include <ostream>
#include <string>
#include <cmath>

using namespace boost;

typedef dynamic_bitset<> db;

typedef typename adjacency_list<vecS, vecS, undirectedS, no_property, property<edge_color_t, int>> Graph;
const int v_size[] = {100,200,300,400,500,1000};
const double l_mul[] = {0.25f,0.50f,1.0f,1.25f};

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

int main()
{
	std::time_t now = std::time(0);
	random::mt19937 gen{ static_cast<std::uint32_t>(now) };

	int size = sizeof(v_size) / sizeof(v_size[0]);
	for (int i = 0; i < size;++i) {
		int l,v,e;
		v = v_size[i];
		e = (v - 1)*v / 10;
		for (int k = 0; k < 4; ++k) {
			for (int j = 1; j <= 10; ++j) {
				Graph g;
				l = v * l_mul[k];
				random::uniform_int_distribution<> dist{ 0, l-1 };
				variate_generator<random::mt19937&, random::uniform_int_distribution<>> rand_gen(gen,dist);
				int J,k_sup;
				J = 0;
				generate_random_graph(g, v, e, gen, false, false);
				boost::randomize_property<edge_color_t>(g, rand_gen);
				//to fix to force l random colors.
				for (int i = 0; i < l; ++i) {
					auto it = random_edge(g,gen);
					auto color = get(edge_color, g);
					std::cout << it<< color[it] <<std::endl;
					color[it] = i;
					std::cout << it << color[it] << std::endl;
				}
				do {
					k_sup = v/std::pow(2,J++);
				} while (kLSFMVCA(g,k_sup,l) == 1);
				k_sup = v / std::pow(2, --J);
				auto colors = get(edge_color, g);
				dynamic_properties dp;
				dp.property("color",colors);
				std::ofstream tmp(std::to_string(v) + "-" + std::to_string(e) + "-" + std::to_string(l)+"-"
					+ std::to_string(k_sup) +"-"+ std::to_string(j) + ".graphml");
				write_graphml(tmp, g, dp, true);
			}
		}
	}
	return 0;
}
