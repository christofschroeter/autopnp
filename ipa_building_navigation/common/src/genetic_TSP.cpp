#include <ipa_building_navigation/genetic_TSP.h>

//Default constructor
GeneticTSPSolver::GeneticTSPSolver()
{

}

////Function to construct the distance matrix from the given points. See the definition at solveGeneticTSP for the style of this matrix.
//void GeneticTSPSolver::constructDistanceMatrix(cv::Mat& distance_matrix, const cv::Mat& original_map, const int number_of_nodes,
//        const std::vector<cv::Point>& points, double downsampling_factor, double robot_radius, double map_resolution)
//{
//	//create the distance matrix with the right size
//	cv::Mat pathlengths(cv::Size(number_of_nodes, number_of_nodes), CV_64F);
//
//	for (int i = 0; i < points.size(); i++)
//	{
//		cv::Point current_center = points[i];
//		for (int p = 0; p < points.size(); p++)
//		{
//			if (p != i)
//			{
//				if (p > i) //only compute upper right triangle of matrix, rest is symmetrically added
//				{
//					cv::Point neighbor = points[p];
//					double length = pathplanner_.planPath(original_map, current_center, neighbor, downsampling_factor, robot_radius, map_resolution);
//					pathlengths.at<double>(i, p) = length;
//					pathlengths.at<double>(p, i) = length; //symmetrical-Matrix --> saves half the computationtime
//				}
//			}
//			else
//			{
//				pathlengths.at<double>(i, p) = 0;
//			}
//		}
//	}
//
//	distance_matrix = pathlengths.clone();
//}

// This function calculates for a given path the length of it. The Path is a vector with ints, that show the index of the
// node in the path. It uses a pathlength Matrix, which should be calculated once.
// This Matrix should save the pathlengths with this logic:
//		1. The rows show from which Node the length is calculated.
//		2. For the columns in a row the Matrix shows the distance to the Node in the column.
//		3. From the node to itself the distance is 0.
double GeneticTSPSolver::getPathLength(const cv::Mat& path_length_Matrix, std::vector<int> given_path)
{
	double length_of_given_path = 0;

	for (int i = 0; i < given_path.size() - 1; i++)
	{
		length_of_given_path += path_length_Matrix.at<double>(given_path[i], given_path[i + 1]);
	}

	return length_of_given_path;
}

// This Function takes the given path and mutates it. A mutation is a random change of the path-order. For example random
// nodes can be switched, or a random intervall of nodes can be inverted. Only the first and last Node can't be changed, because
// they are given from the Main-function.
std::vector<int> GeneticTSPSolver::mutatePath(const std::vector<int>& parent_path)
{
	std::vector<int> mutated_path;

	std::vector<int> temporary_path;
	std::vector<int> saving_variable_path = parent_path; //saving-variable for the one time mutated path

	//this variable sets which aspect should be changed:
	//		0: random nodes should be switched
	//		1: random intervall should be inverted:
	int what_to_change = (rand() % 2);

	if (what_to_change == 0) //random node-switching
	{
		int number_of_switches = (rand() % (parent_path.size() - 3)) + 1; // Set the number of switches that should be done.
		                                                                  // Because the first and the last need to be unchanged the number is limited.
		                                                                  // Also at least one change should be done.
		for (int change = 0; change < number_of_switches; change++)
		{
			temporary_path.clear(); //reset the temporary path to fill it with new order
			bool switched = false; //this variable makes sure that the switch has been done
			do
			{
				int node_one = (rand() % (saving_variable_path.size() - 2)) + 1; //this variables random choose which nodes should be changed
				int node_two = (rand() % (saving_variable_path.size() - 2)) + 1; //The first and last one should be untouched
				if (node_one != node_two) //node can't be switched with himself
				{
					for (int node = 0; node < saving_variable_path.size(); node++) //fill the mutated path with the information
					{
						if (node == node_one)
						{
							temporary_path.push_back(saving_variable_path[node_two]); //add the second node which should be switched
						}
						else if (node == node_two)
						{
							temporary_path.push_back(saving_variable_path[node_one]); //add the first node which should be switched
						}
						else
						{
							temporary_path.push_back(saving_variable_path[node]); //add the nodes as they are
						}
					}
					switched = true;
				}
			} while (!switched);
			saving_variable_path = temporary_path; //save the one time mutated path
		}
		mutated_path = saving_variable_path; //save the finished mutated path
	}
	else if (what_to_change == 1) //random intervall-inverting (always from middle node on)
	{
		bool inverted = false;
		do
		{
			int node_one = (rand() % (saving_variable_path.size() - 2)) + 1; //this variables random choose which intervall
			int node_two = (rand() % (saving_variable_path.size() - 2)) + 1; //The first and last one should be untouched
			int inverting_counter = 0; //variable to choose the node based on distance to the node_two
			if (node_one > node_two) //switch variables, if the node_one is bigger than the node_two (easier to work with here)
			{
				int tmp_node = node_one;
				node_one = node_two;
				node_two = tmp_node;
			}
			if (node_one != node_two)
			{
				for (int node = 0; node < parent_path.size(); node++)
				{
					if (node < node_one || node > node_two) //add the nodes as they are in the mutated path
					{
						mutated_path.push_back(parent_path[node]);
					}
					else //invert the intervall
					{
						mutated_path.push_back(parent_path[node_two - inverting_counter]);
						inverting_counter++;
					}
				}
				inverted = true;
			}
		} while (!inverted);
	}
	else
	{
		ROS_INFO("Something was wrong in mutation-function.");
	}

	return mutated_path;
}

//This Function calculates the length of each given path and chooses the shortest one. It uses the getPathLength function.
std::vector<int> GeneticTSPSolver::getBestPath(const std::vector<std::vector<int> > paths, const cv::Mat& pathlength_Matrix, bool& changed)
{
	std::vector<int> best_path = paths[0];

	double best_distance = getPathLength(pathlength_Matrix, paths[0]); //saving-variable for the distance of the current best path

	for (int current_path = 1; current_path < paths.size(); current_path++)
	{
		double current_distance = getPathLength(pathlength_Matrix, paths[current_path]); //get distance of current path
		if (current_distance < best_distance)
		{
			best_distance = current_distance;
			best_path = paths[current_path];
			changed = true;
		}
	}

	return best_path;
}

//This is a solver for the TSP using a genetic algorithm. It calculates a initial path by using the nearest-neighbor
//search. It then applies an evolutional algorithm:
//
//	I. Take the parent of the current generation and calculate 8 mutated children of it. A mutation can be a change
//	   of positions of nodes or the inversion of a intervall. The initial parent is the path from the nearest-neighbor search.
//	II. It checks for the 9 paths (parent and children) for the best path (shortest) and takes this path as the parent
//	   of the new generation.
//	III. It repeats the steps I. and II. at least a specified amount of times and then checks if the pathlength
//		 hasn't changed in the last steps.
//
//As input a symmetrical matrix of pathlenghts is needed. This matrix should save the pathlengths with this logic:
//		1. The rows show from which Node the length is calculated.
//		2. For the columns in a row the Matrix shows the distance to the Node in the column.
//		3. From the node to itself the distance is 0.

//don't compute distance matrix
std::vector<int> GeneticTSPSolver::solveGeneticTSP(const cv::Mat& path_length_Matrix, const int start_Node)
{
	NearestNeighborTSPSolver nearest_neighbor_solver;

	std::vector<int> calculated_path = nearest_neighbor_solver.solveNearestTSP(path_length_Matrix, start_Node);

	bool changed_path = false; //this variable checks if the path has been changed in the mutation process
	int changeing_counter = 25; //this variable is a counter for how many times a path has been the same

	int number_of_generations = 0;

	do
	{
		number_of_generations++;
		changed_path = false;
		std::vector < std::vector<int> > current_generation_paths; //vector to save the current generation
		current_generation_paths.push_back(calculated_path); //first path is always the parent --> important for checking if the path has changed in getBestPath!!
		for (int child = 0; child < 8; child++) //get 8 children and add them to the vector
		{
			current_generation_paths.push_back(mutatePath(calculated_path));
		}
		calculated_path = getBestPath(current_generation_paths, path_length_Matrix, changed_path); //get the best path of this generation
		if (number_of_generations >= 350) //when 350 steps has been done the algorithm checks if the last ten paths didn't change
		{
			if (changed_path)
			{
				changeing_counter = 25; //reset the counting-variable
			}
			else
			{
				changeing_counter -= 1; //decrease the counting-variable by 1
			}
		}
	} while (changeing_counter > 0 || number_of_generations < 350);

	return calculated_path;
}

//compute distance matrix and maybe returning it
std::vector<int> solveGeneticTSP(const cv::Mat& original_map, const int number_of_nodes, const std::vector<cv::Point>& points, double downsampling_factor,
        double robot_radius, double map_resolution, const int start_Node, cv::Mat* distance_matrix = 0)
{
	//calculate the distance matrix
	cv::Mat distance_matrix_ref;
	if (distance_matrix != 0)
		distance_matrix_ref = *distance_matrix;
	DistanceMatrix::constructDistanceMatrix(distance_matrix_ref, original_map, points, downsampling_factor, robot_radius, map_resolution, pathplanner_);

	return (solveGeneticTSP(distance_matrix_ref, start_Node));
}

