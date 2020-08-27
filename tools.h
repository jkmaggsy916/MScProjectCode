#pragma once
#include <QVector>
#include <QCoreApplication>
#include <QtGlobal>
#include <iostream>
#include <fstream>

class Plane
{
public:

	Plane() {};
	QVector<float> x_postitions;
	QVector<float> y_postitions;
	QVector<float> u_textures;
	QVector<float> v_textures;
	QVector<float> triangles;

	void generateMesh(int width, int height, float spacing) {
		float x_pos, y_pos, u_tex, v_tex;
		float x_offset = 0.5 * (float)width;
		float y_offset = 0.5 * (float)height;
		for (float x = 0.0f; x <= (float)width; x += spacing) {
			x_pos = x - x_offset;
			u_tex = x / (float)width;
			x_postitions.push_back(x_pos);
			u_textures.push_back(u_tex);
		}

		for (float y = 0.0f; y <= (float)height; y += spacing) {
			y_pos = y - y_offset;
			v_tex = y / (float)height;
			y_postitions.push_back(y_pos);
			v_textures.push_back(v_tex);
		}

		for (int i = 0; i < x_postitions.size() - 1; i++) {
			for (int j = 0; j < y_postitions.size() - 1; j++) {
				//lower left point
				triangles.push_back(x_postitions[i]);
				triangles.push_back(y_postitions[j]);
				triangles.push_back(0.0);
				triangles.push_back(u_textures[i]);
				triangles.push_back(v_textures[j]);
				triangles.push_back(1.0);

				//lower right point
				triangles.push_back(x_postitions[i + 1]);
				triangles.push_back(y_postitions[j]);
				triangles.push_back(0.0);
				triangles.push_back(u_textures[i + 1]);
				triangles.push_back(v_textures[j]);
				triangles.push_back(1.0);

				//upper right point
				triangles.push_back(x_postitions[i + 1]);
				triangles.push_back(y_postitions[j + 1]);
				triangles.push_back(0.0);
				triangles.push_back(u_textures[i + 1]);
				triangles.push_back(v_textures[j + 1]);
				triangles.push_back(1.0);

				//lower left point
				triangles.push_back(x_postitions[i]);
				triangles.push_back(y_postitions[j]);
				triangles.push_back(0.0);
				triangles.push_back(u_textures[i]);
				triangles.push_back(v_textures[j]);
				triangles.push_back(1.0);

				//upper right point
				triangles.push_back(x_postitions[i + 1]);
				triangles.push_back(y_postitions[j + 1]);
				triangles.push_back(0.0);
				triangles.push_back(u_textures[i + 1]);
				triangles.push_back(v_textures[j + 1]);
				triangles.push_back(1.0);

				//upper left point
				triangles.push_back(x_postitions[i]);
				triangles.push_back(y_postitions[j + 1]);
				triangles.push_back(0.0);
				triangles.push_back(u_textures[i]);
				triangles.push_back(v_textures[j + 1]);
				triangles.push_back(1.0);
			}
		}
	}
};

#define PI 3.14159265359

class Sphere
{
public:
	Sphere() {};
	QVector<float> vertices;
	QVector<float> indices;
	QVector<float> triangles;

	void generateSphere(float radius, int sectors, int stacks) {
		float x, y, z, xy;

		float sectorStep = 2 * PI / sectors;
		float stackStep = PI / stacks;
		float sectorAngle, stackAngle;
		int k1, k2;

		for (float i = 0; i <= stacks; ++i)
		{
			stackAngle = PI / 2.0f - i * stackStep;
			xy = radius * cosf(stackAngle);
			z = radius * sinf(stackAngle);

			for (int j = 0; j <= sectors; ++j)
			{
				sectorAngle = j * sectorStep;

				x = xy * cosf(sectorAngle);
				y = xy * sinf(sectorAngle);
				vertices.push_back(x);
				vertices.push_back(y);
				vertices.push_back(z);
			}
		}
		
		for (int i = 0; i < stacks; ++i)
		{
			k1 = i * (sectors + 1);
			k2 = k1 + sectors + 1;

			for (int j = 0; j < sectors; ++j, ++k1, ++k2)
			{
				if (i != 0)
				{
					indices.push_back(k1 - 1);
					indices.push_back(k2 - 1);
					indices.push_back(k1);
				}

				if (i != (stacks - 1))
				{
					indices.push_back(k1 );
					indices.push_back(k2 - 1);
					indices.push_back(k2);
				}
			}
		}

		for (int i = 0; i < indices.size(); i ++)
		{
			/*triangles.push_back(vertices[i*3 + 0]);triangles.push_back(vertices[i*3 + 2]);
			triangles.push_back(vertices[i*3 + 1]);*/
			
			triangles.push_back(vertices[3*indices[i] + 0]);
			triangles.push_back(vertices[3*indices[i] + 1]);
			triangles.push_back(vertices[3*indices[i] + 2]);
			triangles.push_back(0.0);	// textures
			triangles.push_back(0.0);	// textures
			triangles.push_back(2.0);	// type
		}

		ofstream myfile;
		myfile.open("vertices.obj");
		for (int i = 0; i < vertices.size() / 3; i++) {
			myfile << "v ";
			myfile << vertices[3 * i + 0] << " ";
			myfile << vertices[3 * i + 1] << " ";
			myfile << vertices[3 * i + 2];
			myfile << "\n";
		}
		for (int i = 0; i < indices.size() / 3; i++) {
			myfile << "f ";
			myfile << indices[3*i + 0] << " ";
			myfile << indices[3*i + 1] << " ";
			myfile << indices[3*i + 2];
			myfile << "\n";
		}
		myfile.close();
	}
};