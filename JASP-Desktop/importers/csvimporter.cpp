//
// Copyright (C) 2013-2016 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "csvimporter.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

#include "sharedmemory.h"
#include "dataset.h"
#include "csv.h"

using boost::lexical_cast;
using namespace boost::interprocess;
using namespace std;

void CSVImporter::loadDataSet(DataSetPackage *packageData, const string &locator, boost::function<void(const string &, int)> progressCallback)
{
	packageData->isArchive = false;

	CSV csv(locator);
	csv.open();

	vector<string> columns = vector<string>();
	vector<vector<string> > cells = vector<vector<string> >();

	csv.readLine(columns);

	unsigned long long progress;
	unsigned long long lastProgress = -1;

	size_t columnCount = columns.size();

	for (size_t i = 0; i < columnCount; i++)  // columns
		cells.push_back(vector<string>());

	vector<string> line;
	bool success = csv.readLine(line);

	while (success)
	{
		progress = 50 * csv.pos() / csv.size();
		if (progress != lastProgress)
		{
			progressCallback("Loading Data Set", progress);
			lastProgress = progress;
		}

        if (line.size() != 0) {
			size_t i = 0;
            for (; i < line.size() && i < columnCount; i++)
                cells[i].push_back(line[i]);
            for (; i < columnCount; i++)
                cells[i].push_back(string());
        }

		line.clear();
		success = csv.readLine(line);
	}

	packageData->dataSet = SharedMemory::createDataSet(); // this is required incase the loading of the data fails so that the SharedMemory::createDataSet() can be later freed.

	do
	{
		try {

			success = true;

			DataSet *dataSet = packageData->dataSet;
			dataSet->setColumnCount(columnCount);
			if (cells.size() > 0)
				dataSet->setRowCount(cells.at(0).size());

		}
		catch (boost::interprocess::bad_alloc &e)
		{
			try {

				packageData->dataSet = SharedMemory::enlargeDataSet(packageData->dataSet);
				success = false;
			}
			catch (exception &e)
			{
				throw runtime_error("Out of memory: this data set is too large for your computer's available memory");
			}
		}
		catch (exception &e)
		{
			cout << "n " << e.what() << "\n";
			cout.flush();
		}
		catch (...)
		{
			cout << "something else\n ";
			cout.flush();
		}
	}
	while ( ! success);


	for (int colNo = 0; colNo < packageData->dataSet->columnCount(); colNo++)
	{
		bool success;

		do {

			success = true;

			try {
				DataSet *dataSet = packageData->dataSet;

				progressCallback("Loading Data Set", 50 + 50 * colNo / dataSet->columnCount());

				string columnName = columns.at(colNo);

				if (columnName == "")
				{
					stringstream ss;
					ss << "V";
					ss << (colNo + 1);
					columnName = ss.str();
				}

				Column &column = dataSet->column(colNo);
				initColumn(column, columnName, cells.at(colNo));

			}
			catch (boost::interprocess::bad_alloc &e)
			{
				try {

					packageData->dataSet = SharedMemory::enlargeDataSet(packageData->dataSet);
					success = false;
				}
				catch (exception &e)
				{
					throw runtime_error("Out of memory: this data set is too large for your computer's available memory");
				}
			}
			catch (exception e)
			{
				cout << "n " << e.what();
				cout.flush();
			}
			catch (...)
			{
				cout << "something else\n ";
				cout.flush();
			}

		} while (success == false);
	}
}


void CSVImporter::initColumn(Column &column, const string &name, const vector<string> &cells)
{
	// we treat single spaces as missing values, because SPSS saves missing values as a single space in CSV files

	column.setName(name);

	// try to make the column nominal

	bool success = true;
	set<int> uniqueValues;
	vector<int> intValues;

	BOOST_FOREACH(const string &value, cells)
	{
		if (value != "NaN" && value != "nan" && value != "" && value != " ")
		{
			try
			{
				int v = lexical_cast<int>(value);
				uniqueValues.insert(v);
				intValues.push_back(v);
			}
			catch (...)
			{
				// column can't be made nominal numeric

				success = false;
				break;
			}
		}
		else
		{
			intValues.push_back(INT_MIN);
		}
	}

	if (success && uniqueValues.size() <= 24)
	{
		column.setColumnAsNominalOrOrdinal(intValues, uniqueValues);
		return;
	}

	// try to make the column scale
	success = true;
	vector<double> doubleValues;

	BOOST_FOREACH(const string &value, cells)
	{
		string v = deEuropeanise(value);
		double doubleValue;

		if (v != "" && v != " ")
		{
			try
			{
				doubleValue = lexical_cast<double>(v);
			}
			catch (...)
			{
				// column can't be made scale
				success = false;
				break;
			}
		}
		else
		{
			doubleValue = NAN;
		}

		doubleValues.push_back(doubleValue);
	}

	if (success)
	{
		column.setColumnAsScale(doubleValues);
		return;
	}

	// if it can't be made nominal numeric or scale, make it nominal-text
	column.setColumnAsNominalString(cells);
}


string CSVImporter::deEuropeanise(const string &value)
{
	int dots = 0;
	int commas = 0;

	for (size_t i = 0; i < value.length(); i++)
	{
		if (value[i] == '.')
			dots++;
		else if (value[i] == ',')
			commas++;
	}

	if (commas > 0)
	{
		string uneurope = value;

		if (dots > 0)
		{
			size_t i = 0;
			size_t j = 0;

			for (;i < value.size(); i++)
			{
				if (value[i] == '.')
					continue;
				uneurope[j] = value[i];

				j++;
			}

			uneurope.resize(j);
		}

		for (size_t i = 0; i < uneurope.length(); i++)
		{
			if (uneurope[i] == ',')
			{
				uneurope[i] = '.';
				break;
			}
		}

		return uneurope;
	}

	return value;
}

