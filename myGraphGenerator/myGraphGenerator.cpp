#include <iostream>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphml.hpp>
#include <boost/graph/random.hpp>
#include <boost/random.hpp>

#include <ctime>
#include <cstdint>
#include <ostream>
#include <string>
using namespace boost;



typedef typename adjacency_list<vecS, vecS, undirectedS, no_property, property<edge_color_t, int>> Graph;

const int v_size[] = {100,200,300,400,500,1000};
const double l_mul[] = {0.25f,0.50f,1.0f,1.25f};

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
				generate_random_graph(g, v, e, gen, false, false);
				boost::randomize_property<edge_color_t>(g,rand_gen);
				//remember to fix to force l random colors and put k verification to mvca folowing consoli patern of tests.

				auto colors = get(edge_color, g);
				dynamic_properties dp;
				dp.property("color",colors);
				std::ofstream tmp(std::to_string(v) + "-" + std::to_string(e) + "-" + std::to_string(l)+"-"
					+ std::to_string(j) + ".graphml");
				write_graphml(tmp, g, dp, true);
			}
		}
	}
	return 0;
}
