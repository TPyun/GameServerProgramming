#pragma once
#include <vector>
#include <queue>
#include <cmath>
#include "../Protocol.h"

using namespace std;

struct Node {
	int x, y;        // Coordinates of the cell
	int g;           // Cost from the start node to this node
	int h;           // Heuristic cost from this node to the target node
	int f;           // Total cost of the node (f = g + h)
	bool obstacle;   // Flag to indicate if the cell is an obstacle
	Node* parent;    // Pointer to the parent node

	Node() {};
	Node(int x, int y) : x(x), y(y), g(0), h(0), f(0), obstacle(false), parent(nullptr) {}

	// Calculate the heuristic cost (Euclidean distance)
	int calculateHeuristic(const Node& goal) {
		return sqrt(pow(x - goal.x, 2) + pow(y - goal.y, 2));
	}
};

// Custom comparator for the priority queue
struct CompareNodes {
	bool operator()(const Node* a, const Node* b) {
		return a->f > b->f;
	}
};

// A* search algorithm
vector<Node*> aStar(Node** grid, const Node& start, const Node& goal, int width, int height) {
	priority_queue<Node*, vector<Node*>, CompareNodes> openNodes;
	openNodes.push(&grid[start.x][start.y]);

	vector<vector<bool>> visited(width, vector<bool>(height, false));

	while (!openNodes.empty()) {
		Node* current = openNodes.top();
		openNodes.pop();
		
		if (current->x == goal.x && current->y == goal.y) {
			vector<Node*> path;
			Node* node = current;
			while (node != nullptr) {
				path.push_back(node);
				node = node->parent;
			}
			return path;
		}

		visited[current->x][current->y] = true;
		vector<Node*> neighbors;

		// Top neighbor
		if (current->y > 0 && !grid[current->x][current->y - 1].obstacle) {
			neighbors.push_back(&grid[current->x][current->y - 1]);
		}
		//Top left neighbor
		if (current->x > 0 && current->y > 0 && !grid[current->x - 1][current->y - 1].obstacle) {
			neighbors.push_back(&grid[current->x - 1][current->y - 1]);
		}
		// Top right neighbor
		if (current->x < width - 1 && current->y > 0 && !grid[current->x + 1][current->y - 1].obstacle) {
			neighbors.push_back(&grid[current->x + 1][current->y - 1]);
		}
		// Left neighbor
		if (current->x > 0 && !grid[current->x - 1][current->y].obstacle) {
			neighbors.push_back(&grid[current->x - 1][current->y]);
		}
		// Right neighbor
		if (current->x < width - 1 && !grid[current->x + 1][current->y].obstacle) {
			neighbors.push_back(&grid[current->x + 1][current->y]);
		}
		// Bottom neighbor
		if (current->y < height - 1 && !grid[current->x][current->y + 1].obstacle) {
			neighbors.push_back(&grid[current->x][current->y + 1]);
		}
		// Bottom left neighbor
		if (current->x > 0 && current->y < height - 1 && !grid[current->x - 1][current->y + 1].obstacle) {
			neighbors.push_back(&grid[current->x - 1][current->y + 1]);
		}
		// Bottom right neighbor
		if (current->x < width - 1 && current->y < height - 1 && !grid[current->x + 1][current->y + 1].obstacle) {
			neighbors.push_back(&grid[current->x + 1][current->y + 1]);
		}

		for (Node* neighbor : neighbors) {
			int gCost = current->g + 1;

			if (!visited[neighbor->x][neighbor->y] || gCost < neighbor->g) {
				neighbor->g = gCost;
				neighbor->h = neighbor->calculateHeuristic(goal);
				neighbor->f = neighbor->g + neighbor->h;
				neighbor->parent = current;
				openNodes.push(neighbor);
				visited[neighbor->x][neighbor->y] = true;
			}
		}
	}

	// No path found
	return vector<Node*>();
}

void printGrid(Node** grid, int width, int height) {
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (grid[x][y].obstacle) {
				cout << "# ";
			}
			else {
				cout << ". ";
			}
		}
		cout << endl;
	}
}

TI turn_astar(TI start_point, TI target_point, TI diff)
{
	int width = diff.x;
	int height = diff.y;
	
	Node** grid = new Node * [width];
	for (int x = 0; x < width; ++x) {
		grid[x] = new Node[height];
		for (int y = 0; y < height; ++y) {
			grid[x][y] = Node(x, y);
		}
	}

	//// Add obstacles to the grid
	//grid[2][3].obstacle = true;
	//grid[3][3].obstacle = true;
	//grid[4][3].obstacle = true;
	
	// Define the start and target nodes
	Node start(start_point.x, start_point.y);
	Node target(target_point.x, target_point.y);

	// Run the A* algorithm
	vector<Node*> path = aStar(grid, start, target, width, height);
	if (path.size() <= 1)
	{
		cout << "같은 위치\n";
		return { -1,-1};
	}

	// Print the grid with the path opposite
	/*for (auto it = path.rbegin(); it != path.rend(); ++it) {
		Node* node = *it;
		grid[node->x][node->y].obstacle = true;
		printGrid(grid, width, height);
		grid[node->x][node->y].obstacle = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		system("cls");
	}*/
	auto next_point = path.rbegin();
	Node* node = *++next_point;
	//cout << "Next point: " << node->x << " " << node->y << endl;
	TI r_next_point = { node->x, node->y };
	// Clean up memory
	for (int x = 0; x < width; ++x) {
		delete[] grid[x];
	}
	delete[] grid;
	
	return r_next_point;
}
