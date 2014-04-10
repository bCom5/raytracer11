#include "parallel_tiles_renderer.h"

namespace raytracer11
{

	void parallel_tiles_renderer::render_tile(uvec2 pos)
	{
		ray r(vec3(0), vec3(0));
		hit_record hr(10000.f);
		for (float y = pos.y; y < pos.y + _tilesize.y && y < rt->size().y; ++y)
		{
			for (float x = pos.x; x < pos.x + _tilesize.x && x < rt->size().x; ++x)
			{
				r = _c.generate_ray(vec2(x, y));
				hr.t = 100000.f;
				rt->pixel(uvec2(x, y)) = raycolor(r);
			}
		}
	}

	void parallel_tiles_renderer::render_tile_aa(uvec2 pos)
	{
		float smpl = (1.f / (float)_samples);
		ray r(vec3(0), vec3(0));
		hit_record hr(10000.f);
		for (float y = pos.y; y < pos.y + _tilesize.y && y < rt->size().y; ++y)
		{
			for (float x = pos.x; x < pos.x + _tilesize.x && x < rt->size().x; ++x)
			{
				vec3 fc = vec3(0);
				for (float p = 0; p < _samples; ++p)
				{
					for (float q = 0; q < _samples; ++q)
					{
						vec2 aaof = (vec2(p, q) + linearRand(vec2(-1), vec2(1))) * smpl;
						r = _c.generate_ray(vec2(x, y)+aaof);
						hr.t = 100000.f;
						fc += raycolor(r);
					}
				}
				rt->pixel(uvec2(x, y)) = fc * (smpl*smpl);
			}
		}
	}

	void parallel_tiles_renderer::render()
	{
		mutex tile_queue_mutex;
		queue<uvec2> tiles;

		vector<uvec2> start_tiles;

		for (uint y = 0; y < rt->size().y; y += _tilesize.y)
		{
			for (uint x = 0; x < rt->size().x; x += _tilesize.x)
			{
				start_tiles.push_back(uvec2(x, y));
			}
		}

		for (int i = 0; i < 20 + start_tiles.size(); ++i)
			swap(start_tiles[rand() % start_tiles.size()], start_tiles[rand() % start_tiles.size()]);

		for (const auto& t : start_tiles)
			tiles.push(t);

		vector<thread> threads;
		for (int ti = 0; ti < _numthreads; ++ti)
		{
			if(_samples == 0)
			{			
				threads.push_back(thread([&] {
					uint tiles_rendered = 0;
					bool notdone = true;
					auto start_time = chrono::system_clock::now();
					while(notdone)
					{
						uvec2 t;
						{
							unique_lock<mutex> lm(tile_queue_mutex);
							if (tiles.empty()) { notdone = false; break; }
							t = tiles.front(); tiles.pop();
						}
						render_tile(t);
						tiles_rendered++;
					}
					auto end_time = chrono::system_clock::now();
					long tm = chrono::duration_cast<chrono::nanoseconds>(end_time - start_time).count();
					auto tmpt = (double)tm / (double)tiles_rendered;
					cout << "thread " << this_thread::get_id() << " rendered " << tiles_rendered << " tiles"
							<< " took " << (double)tm/1000000.0 << "ms, " << tmpt/1000000.0 << "ms/tile" << endl;
				}));
			}
			else
			{
				threads.push_back(thread([&] {
					uint tiles_rendered = 0;
					bool notdone = true;
					auto start_time = chrono::system_clock::now();
					while (notdone)
					{
						uvec2 t;
						{
							unique_lock<mutex> lm(tile_queue_mutex);
							if (tiles.empty()) { notdone = false; break; }
							t = tiles.front(); tiles.pop();
						}
						render_tile_aa(t);
						tiles_rendered++;
					}
					auto end_time = chrono::system_clock::now();
					long tm = chrono::duration_cast<chrono::nanoseconds>(end_time - start_time).count();
					auto tmpt = (double)tm / (double)tiles_rendered;
					cout << "thread " << this_thread::get_id() << " rendered " << tiles_rendered << " tiles"
						<< " took " << (double)tm / 1000000.0 << "ms, " << tmpt / 1000000.0 << "ms/tile" << endl;
				}));
			}
		}

		for(auto& t : threads)
		{
			t.join();
			this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}
}