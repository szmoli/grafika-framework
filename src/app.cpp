//=============================================================================================
// Z�ld h�romsz�g: A framework.h oszt�lyait felhaszn�l� megold�s
//=============================================================================================
#include "../inc/framework.h"
#include <vector>
#include <sstream>

using namespace std;

string vec3ToString(const vec3& v) {
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "(%f, %f, %f)", v.x, v.y, v.z);
    return string(buffer);
}

enum class State {
	POINT,
	LINE,
	MOVE,
	INTERSECT
};

class Object {
public:
	Object() {
		vertices = vector<vec3>();
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}

	~Object() {
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}

	virtual void sync() {
		bind();
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), vertices.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
	}

	virtual void render(const unsigned int primitiveType, GPUProgram *gpuProg, const vec3& color) {
		bind();
		gpuProg->setUniform(color, "color");
		glDrawArrays(primitiveType, 0, vertices.size());
	}

	unsigned int getVao() {
		return vao;
	}

	unsigned int getVbo() {
		return vbo;
	}

protected:
	std::vector<vec3> vertices;
	
	void addVertex(const vec3& vert) {
		vertices.push_back(vert);
	}

	void bind() {
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}

private:
	unsigned int vao;
	unsigned int vbo;
};

class PointCollection : public Object {
public:
	PointCollection() : Object() {}

	void addPoint(vec3 r) {
		Object::addVertex(r);
		printf("Point added: %s\n", vec3ToString(r).c_str());
	}

	unsigned int getPointCount() {
		return vertices.size();
	} 

	vec3* proximitySearch(const vec3& p, float d) {
		unsigned int i = 0;
		while (i < vertices.size()) {
			float distance = sqrt(pow((vertices[i].x - p.x), 2) + pow((vertices[i].y - p.y), 2));

			if (distance <= d) {
				return (vec3*) (vertices.data() + i);
			}

			++i;
		}

		return nullptr;
	}

private:
};

class Line {
public:
	Line(const vec3 &p, const vec3 &q) {
		this->p = p;
		this->q = q;
	}

	vec3 intersection(const Line& other) const {
		vec3 e_n = this->normal();
		vec3 f_n = other.normal();
		float det = e_n.x * f_n.y - e_n.y * f_n.x;

		// std::cout << "det: " << std::abs(det) << std::endl;
		
		if (std::abs(det) < 0.01f) {
			// std::cout << "det wasn't large enough" << std::endl;
			return vec3(-0.0f, -0.0f, 1.0f);
		}

		float C_e = (e_n.x * this->p.x + e_n.y * this->p.y);
		float C_f = (f_n.x * other.p.x + f_n.y * other.p.y);
		
		return vec3(
			(f_n.y * C_e - e_n.y * C_f) / det,
			(e_n.x * C_f - f_n.x * C_e) / det,
			1.0f
		);
	}

	bool isPointOnLine(const vec3& point) const {
		return distanceFromLine(point) <= 0.01f;
	}

	float distanceFromLine(const vec3& point) const {
		vec3 n = normal();
		float C = -(n.x * p.x + n.y * p.y);
		return abs(n.x * point.x + n.y * point.y + C) / length(n);
	}

	void move(const vec3& cP) {
		vec3 offset = cP - p;
		p += offset;
		q += offset; 
	}

	vec3 getP() {
		return p;
	}

	vec3 getQ() {
		return q;
	}

	vec3 direction() const {
		return q - p;
	}

	vec3 normal() const {
		vec3 v = direction();
		return vec3(-v.y, v.x, v.z);
	}

	string getParametricEquation() {
		char buffer[128];
		string pStr = vec3ToString(p);
		string dirStr = vec3ToString(direction());
		snprintf(buffer, sizeof(buffer), "r(t) = %s + %s * t", pStr.c_str(), dirStr.c_str());
		return string(buffer);
	}

	string getImplicitEquation() {
		char buffer[128];
		vec3 n = normal();
		snprintf(buffer, sizeof(buffer), "%f * x + %f * y + %f = 0", n.x, n.y, -(n.x * p.x + n.y * p.y));
		return string(buffer);
	}

	void printEquations() {
		printf("\tImplicit: %s\n\tParametric: %s\n", getImplicitEquation().c_str(), getParametricEquation().c_str());
	}

private:
	vec3 p;
	vec3 q;
};

class LineCollection : public Object {
public:
	LineCollection() : Object() {
		lines = vector<Line>();
	}

	void addLine(const vec3& p, const vec3& q) {
		Line l = Line(p, q);
		lines.push_back(l);
		printf("Added line:\n");
		l.printEquations();
	}

	Line* getLineAtPos(const vec3& p) {
		for (Line& line : lines) {
			if (line.isPointOnLine(p)) {
				return &line;
			}
		}

		return nullptr;
	}

	void sync() override {
		Line boundries[4] = {
			Line(vec3(-1.0f, 1.0f, 1.0f), vec3(1.0f, 1.0f, 1.0f)),   // top
			Line(vec3(-1.0f, -1.0f, 1.0f), vec3(1.0f, -1.0f, 1.0f)), // bottom
			Line(vec3(-1.0f, 1.0f, 1.0f), vec3(-1.0f, -1.0f, 1.0f)), // left
			Line(vec3(1.0f, 1.0f, 1.0f), vec3(1.0f, -1.0f, 1.0f))    // right
		};
	
		vertices.clear();
	
		for (const Line& line : lines) {
			vec3 intersections[4] = {
				line.intersection(boundries[0]),
				line.intersection(boundries[1]),
				line.intersection(boundries[2]),
				line.intersection(boundries[3])
			};

			vec3 intersection1 = vec3(-0.0f, -0.0f, 1.0f);
			vec3 intersection2 = vec3(-0.0f, -0.0f, 1.0f);

			unsigned int i = 0;
			while (i < 4 && (intersection1.x == -0.0f && intersection1.y == -0.0f)) {
				// std::cout << "i: ";
				// printVec2(intersections[i]);
				if (intersections[i].x != -0.0f && intersections[i].y != -0.0f) {
					// std::cout << "found i" << std::endl;
					intersection1 = intersections[i];
				}
				++i;
			}

			unsigned int j = i;
			while (j < 4 && (intersection2.x == -0.0f && intersection2.y == -0.0f)) {
				// std::cout << "j: ";
				// printVec2(intersections[j]);
				if (intersections[j].x != -0.0f && intersections[j].y != -0.0f) {
					// std::cout << "found j" << std::endl;
					intersection2 = intersections[j];
				}
				++j;
			}

			// printVec2(intersection1);
			// printVec2(intersection2);

			if (intersection1.x != -0.0f && intersection1.y != -0.0f && intersection2.x != -0.0f && intersection2.y != -0.0f) {
				vertices.push_back(intersection1);
				vertices.push_back(intersection2);
			}
		}

		bind();
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), vertices.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
	}
	

private:
	std::vector<Line> lines;
};

// cs�cspont �rnyal�
const char * vertSource = R"(
	#version 330				
    precision highp float;

	layout(location = 0) in vec3 cP;	// 0. bemeneti regiszter

	void main() {
		gl_Position = vec4(cP.x, cP.y, cP.z, 1); 	// bemenet m�r normaliz�lt eszk�zkoordin�t�kban
	}
)";

// pixel �rnyal�
const char * fragSource = R"(
	#version 330
    precision highp float;

	uniform vec3 color;			// konstans sz�n
	out vec4 fragmentColor;		// pixel sz�n

	void main() {
		fragmentColor = vec4(color, 1); // RGB -> RGBA
	}
)";

const int winWidth = 600, winHeight = 600;

class PointsAndLinesApp : public glApp {
	PointCollection *points;
	LineCollection *lines;
	GPUProgram *gpuProgram;	   // cs�cspont �s pixel �rnyal�k
	State state;
	vec3* lineStart;
	vec3* lineEnd;
	Line* selectedLine1;
	Line* selectedLine2;
	bool mouseLeftHeld;

public:
	PointsAndLinesApp() : glApp("Points and lines") { }

	// Inicializ�ci�, 
	void onInitialization() override {
		glPointSize(10);
		glLineWidth(3);
		gpuProgram = new GPUProgram(vertSource, fragSource);
		state = State::POINT;
		mouseLeftHeld = false;
		lineStart = nullptr;
		lineEnd = nullptr;
		selectedLine1 = nullptr;
		points = new PointCollection();
		lines = new LineCollection();
		points->sync();
		lines->sync();		
	}

	// Ablak �jrarajzol�s
	void onDisplay() override {
		glClearColor(0.2f, 0.2f, 0.2f, 0);     // h�tt�r sz�n
		glClear(GL_COLOR_BUFFER_BIT); // rasztert�r t�rl�s
		glViewport(0, 0, winWidth, winHeight);
		lines->render(GL_LINES, gpuProgram, vec3(0.0f, 1.0f, 1.0f));
		points->render(GL_POINTS, gpuProgram, vec3(1.0f, 0.0f, 0.0f));
	}

	virtual void onMousePressed(MouseButton but, int pX, int pY) {
		if (but != MouseButton::MOUSE_LEFT) {
			return;
		}

		// std::cout << "Holding mouse left button" << std::endl;
		mouseLeftHeld = true;
		vec3 cP = toNormalizedDeviceSpace(vec3(pX, pY, 1.0f));

		switch (state) {
			case State::POINT:
				points->addPoint(cP);
				points->sync();
				refreshScreen();
				break;

			case State::LINE:
				if (lineStart == nullptr) {
					lineStart = points->proximitySearch(cP, 0.01f);
				} else {
					lineEnd = points->proximitySearch(cP, 0.01f);
				}

				if (lineStart != nullptr && lineEnd != nullptr) {
					lines->addLine(*lineStart, *lineEnd);
					lines->sync();
					refreshScreen();
					lineStart = nullptr;
					lineEnd = nullptr;
				}
				break;

			case State::MOVE:
				selectedLine1 = lines->getLineAtPos(cP);
				break;

			case State::INTERSECT:
				if (selectedLine1 == nullptr) {
					selectedLine1 = lines->getLineAtPos(cP);
				} else {
					selectedLine2 = lines->getLineAtPos(cP);
				}

				if (selectedLine1 != nullptr && selectedLine2 != nullptr) {
					points->addPoint(selectedLine1->intersection(*selectedLine2));
					points->sync();
					refreshScreen();
					selectedLine1 = nullptr;
					selectedLine2 = nullptr;
				}
				break;

			default:
				break;
		}
	}

	virtual void onMouseReleased(MouseButton but, int pX, int pY) {
		if (but != MouseButton::MOUSE_LEFT) {
			return;
		}

		mouseLeftHeld = false;
		// std::cout << "Released mouse left button" << std::endl;

		switch (state)
		{
		case State::MOVE:
			printf("Moved line:\n");
			selectedLine1->printEquations();
			selectedLine1 = nullptr;
			break;
		default:
			break;
		}
	}

	vec3 toNormalizedDeviceSpace(const vec3& p_v) {
		return vec3(
			2 * (p_v.x / winWidth - 0.5f),
			2 * (0.5f - p_v.y / winHeight),
			p_v.z
		);
	}

	virtual void onMouseMotion(int pX, int pY) {
		vec3 cP = toNormalizedDeviceSpace(vec3(pX, pY, 1.0f));

		switch (state)
		{
			case State::MOVE:
				if (selectedLine1 == nullptr || !mouseLeftHeld) {
					break;
				}

				selectedLine1->move(cP);
				lines->sync();
				refreshScreen();
				break;
			
			default:
				break;
		}
	}

	void onKeyboard(int key) override {
		switch (key)
		{
			case 'p':
			state = State::POINT;
			break;
			case 'm':
			state = State::MOVE;
			break;
			case 'l':
			state = State::LINE;
			break;
			case 'i':
			state = State::INTERSECT;
			break;
			default:
			break;
		}

		// std::cout << "Key pressed: '" << (char) key << "'" << std::endl << "Switched to mode: " << (int) state << std::endl;
	}   // Klaviat�ra gomb lenyom�s

	virtual void onKeyboardUp(int key) {} // Klaviat�ra gomb elenged
};

PointsAndLinesApp app;

