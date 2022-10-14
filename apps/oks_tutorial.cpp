/************************************************************************
*                                                                       *
* tutorial.cpp                                                          *
*        Explains basics of OKS C++ API including:                      *
*                - kernel initialization                                *
*                - schema design                                        *
*                - data creation                                        *
*                - data manipulation                                    *
*                - notification                                         *
*                                                                       *
* Author Igor Soloviev                                                  *
*                                                                       *
* Created: 14 Oct 1996                                                  *
*                                                                       *
* Modified:                                                             *
*        11 Feb 1998                                                    *
*                - add database quering                                 *
*                                                                       *
************************************************************************/

#include "boost/date_time/gregorian/gregorian.hpp"

#include "oks/kernel.hpp"
#include "oks/object.hpp"
#include "oks/class.hpp"
#include "oks/attribute.hpp"
#include "oks/relationship.hpp"
#include "oks/query.hpp"
#include "oks/exceptions.hpp"


  //
  // This function sets attribute values of class 'Person'
  //

void
setPersonValues(
  OksObject *o,                    // OKS object describing person
  const char *name,                // new person name
  boost::gregorian::date birthday, // new person birthday
  const char *familySituation      // new person family situation
)
{
  OksData d;                       // creates OKS data with unknown type

  d.Set(name);                     // sets OKS data to string 'name'
  o->SetAttributeValue("Name", &d);

  d.Set(birthday);                 // sets OKS data to date 'birthday'
  o->SetAttributeValue("Birthday", &d);

  d.Set(familySituation);          // sets OKS data to 'familySituation'
  d.type = OksData::enum_type;     // sets OKS data type to enumeration
  o->SetAttributeValue("Family Situation", &d);
}


  //
  // This function prints an instance of class 'Person'
  //

void
printPerson(
  const OksObject *o               // OKS object describing person
)
{
  OksData * name(o->GetAttributeValue("Name"));                 // is used to store 'Name'
  OksData * birthday(o->GetAttributeValue("Birthday"));         // is used to store 'Birthday'
  OksData * family(o->GetAttributeValue("Family Situation"));   // is used to store 'Family Situation'

  std::cout << "Object " << o << " \n"
             " Name: " << *name << " \n"
             " Birthday: \'" << *birthday << "\" \n"
             " Family Situation: " << *family << std::endl;
}


  //
  // This function sets attribute values of class 'Employee'
  //

void
setEmployeeValues(
  OksObject *o,                    // OKS object that describes employee
  const char *name,                // new employee name
  boost::gregorian::date birthday, // new employee birthday
  const char *familySituation,     // new employee family situation
  uint32_t salary                  // new employee salary situation
) 
{
    // we can use setPersonValues() because 'Employee' class
    // derived from 'Person' class

  setPersonValues(o, name, birthday, familySituation);

  OksData d(salary);               // creates OKS data with ulong 'salary'
  o->SetAttributeValue("Salary", &d);
}


  //
  // This function prints an instance of class 'Employee'
  //

void
printEmployee(
  const OksObject *o               // OKS object that describes employee
)
{
    // we can use printPerson() because 'Employee' class
    // is derived from 'Person' class

  printPerson(o);

  OksData *department(o->GetRelationshipValue("Works at")),  // is used to store 'Works at'
          *salary(o->GetAttributeValue("Salary"));           // is used to store 'salary'

  std::cout << " Salary: " << *salary << " \n"
               " Works at: \"" << department->data.OBJECT->GetId() << "\"\n";
}


  //
  // This function sets attribute values of class 'Department'
  //

void
setDepartmentValues(
  OksObject *o,                    // OKS object that describes department
  const char *name                 // new department name
) 
{
  OksData d(name);                 // creates OKS data with string 'name'

  o->SetAttributeValue("Name", &d);
}


  //
  // This function prints an instance of class 'Department'
  //

void
printDepartment(
  const OksObject *o               // OKS object that describes department
)
{
  OksData *name(o->GetAttributeValue("Name")),       // is used to store 'Staff'
          *staff(o->GetRelationshipValue("Staff"));  // is used to store 'Name'

  std::cout << "Object " << o << "\n"
               " Name: " << *name << " \n"
               " Staff: \"" << *staff << "\"\n";
}


int
main(int argc, char **argv)
{
  const char * schema_file = "/tmp/tutorial.oks"; // default schema file
  const char * data_file   = "/tmp/tutorial.okd"; // default data file

  if(
    argc > 1 &&
    (
      !strcmp(argv[1], "--help") ||
      !strcmp(argv[1], "-help") ||
      !strcmp(argv[1], "--h") ||
      !strcmp(argv[1], "-h")
    )
  ) {
    std::cout << "Usage: " << argv[0] << " [new_schema new_data]\n";
    return 0;
  }

  if(argc == 3) {
    schema_file = argv[1];
    data_file = argv[2];
  }


    // Creates OKS kernel

  std::cout << "[OKS TUTORIAL]: Creating OKS kernel...\n";

  OksKernel kernel(false, false, false, false);

  std::cout << "[OKS TUTORIAL]: Done creating OKS kernel\n\n";


  try {        


      // Creates new schema file and tests return status

    std::cout << "[OKS TUTORIAL]: Creating new schema file...\n";

    OksFile * schema_h = kernel.new_schema(schema_file);

    std::cout << "[OKS TUTORIAL]: Done creating new schema file...\n\n"
                 "[OKS TUTORIAL]: Define database class schema...\n\n"
                 "  **********        ************     1..1 **************\n"
                 "  * Person *<|------* Employee *--------<>* Department *\n"
                 "  **********        ************ 0..N     **************\n\n";


      // Creates class Person with three attributes "Name", "Birthday" and "Family Situation"

    OksClass * Person = new OksClass(
      "Person",
      "It is a class to describe a person",
      false,
      &kernel
    );

    Person->add(
      new OksAttribute(
        "Name",
        OksAttribute::string_type,
        false,
        "",
        "Unknown",
        "A string to describe person name",
        true
      )
    );

    OksAttribute * PersonBirthday = new OksAttribute(
      "Birthday",
      OksAttribute::date_type,
      false,
      "",
      "2009/01/01",
      "A date to describe person birthday",
      true
    );

    Person->add(PersonBirthday);

    Person->add(
      new OksAttribute(
        "Family Situation",
        OksAttribute::enum_type,
        false,
        "Single,Married,Widow(er)",
        "Single",
        "A enumeration to describe a person family state",
        true
      )
    );


      // Creates class Person with superclass Person, add "Salary" attribute and "Works at" relationship

    OksClass * Employee = new OksClass(
      "Employee",
      "It is a class to describe an employee",
      false,
      &kernel
    );

    OksAttribute * EmployeeSalary = new OksAttribute(
      "Salary",
      OksAttribute::u32_int_type,
      false,
      "",
      "1000",
      "An integer to describe employee salary",
      false
    );

    OksRelationship * WorksAt = new OksRelationship(
      "Works at",
      "Department",
      OksRelationship::One,
      OksRelationship::One,
      false,
      false,
      false,
      "A employee works at one and only one department"
    );
  
    Employee->add_super_class("Person");
    Employee->add(EmployeeSalary);
    Employee->add(WorksAt);


      // Creates class Department with one attribute and one relationship

    OksClass * Department = new OksClass(
      "Department",
      "It is a class to describe a department",
      false,
      &kernel
    );

    OksAttribute * DepartmentName = new OksAttribute(
      "Name",
      OksAttribute::string_type,
      false,
      "",
      "Unknown",
      "A string to describe department name",
      true
    );

    OksRelationship *DepartmentStaff = new OksRelationship(
      "Staff",
      "Employee",
      OksRelationship::Zero,
      OksRelationship::Many,
      true,
      true,
      true,
      "A department has zero or many employess"
    );

    Department->add(DepartmentName);
    Department->add(DepartmentStaff);


      // Saves created schema file

    std::cout << "[OKS TUTORIAL]: Saves created OKS schema file...\n";

    kernel.save_schema(schema_h);


      // Creates new data file and tests return status

    std::cout << "[OKS TUTORIAL]: Creating new data file...\n";

    OksFile * data_h = kernel.new_data(data_file, "OKS TUTORIAL DATA FILE");

    data_h->add_include_file(schema_file);


      // Creates instances of the classes

    OksObject * person1     = new OksObject(Person, "peter");
    OksObject * person2     = new OksObject(Person, "mick");
    OksObject * person3     = new OksObject(Person, "baby");
    OksObject * employee1   = new OksObject(Employee, "alexander");
    OksObject * employee2   = new OksObject(Employee, "michel");
    OksObject * employee3   = new OksObject(Employee, "maria");
    OksObject * department1 = new OksObject(Department, "IT");
    OksObject * department2 = new OksObject(Department, "EP");


      // Sets attribute values for instances of class 'Person'

    setPersonValues(person1, "Peter", boost::gregorian::from_string("1960/02/01"), "Married");
    setPersonValues(person2, "Mick", boost::gregorian::from_string("1956-09-01"), "Single");
    setPersonValues(person3, "Julia", boost::gregorian::from_string("2000-May-25"), "Single");


      // Sets attribute values for instances of class 'Employee'

    setEmployeeValues(employee1, "Alexander", boost::gregorian::from_string("1972/05/12"), "Single", 3540) ;
    setEmployeeValues(employee2, "Michel", boost::gregorian::from_string("1963/01/28"), "Married", 4950) ;
    setEmployeeValues(employee3, "Maria", boost::gregorian::from_string("1951/08/18"), "Widow(er)", 4020) ;


      // Sets attribute values for instance of class 'Department'

    setDepartmentValues(department1, "IT Department"); 
    setDepartmentValues(department2, "EP Department"); 


      // Sets relationships

    department1->AddRelationshipValue("Staff", employee1);
    department1->AddRelationshipValue("Staff", employee2);
    department2->AddRelationshipValue("Staff", employee3);
    employee1->SetRelationshipValue("Works at", department1);
    employee2->SetRelationshipValue("Works at", department1);
    employee3->SetRelationshipValue("Works at", department2);


      // Print out database contents

    std::cout << "\n[OKS TUTORIAL]: Database contains the following data:\n";

    printPerson(person1);
    printPerson(person2);
    printPerson(person3);
    printEmployee(employee1);
    printEmployee(employee2);
    printEmployee(employee3);
    printDepartment(department1);
    printDepartment(department2);
  
  
      // Saves created data file

    std::cout << "\n[OKS TUTORIAL]: Saves created OKS data file...\n";

    kernel.save_data(data_h);


    std::cout << "[OKS TUTORIAL]: Done with saving created OKS data file\n";


      // **********************************************************************
      //   ***** At this moment the database has been created and saved *****
      // **********************************************************************


    std::cout << "\n[OKS TUTORIAL]: Start database querying tests\n\n";


      // This is a test for OksComparator and OksQuery

    {
      std::cout << "[QUERY]: Start simple database querying...\n";

        // Looking for persons were born after 01 January 1960

      boost::gregorian::date aDate(boost::gregorian::from_string("1960/01/01")); // query date
      OksQuery query(true, new OksComparator(PersonBirthday, new OksData(aDate), OksQuery::greater_cmp));
        

        // executes query

      std::cout << "[QUERY]: Looking for persons were born after " << aDate << " ...\n\n";
        
      OksObject::List * queryResult = Person->execute_query(&query);


        // builds iterator over results if something was found

      if(queryResult) {
        std::cout << "[QUERY]: Query \'" << query
                  << "\'\n  founds the following objects in class \'"
                  << Person->get_name() << "\' and subclasses:\n";

        for(OksObject::List::iterator i = queryResult->begin(); i != queryResult->end(); ++i) {
          OksObject * o = *i;
          OksData * d(o->GetAttributeValue("Birthday"));
          std::cout << "   - " << o << " was born " << *d << std::endl;
        }


          // free list: we do not need it anymore

        delete queryResult;
      }
        
      std::cout << "[QUERY]: Done with simple database querying\n\n";
    }
  
    {
      std::cout << "[QUERY]: Start database querying with logical function...\n";


          // Looking for persons were born after 01 January 1960
          // and before 01 January 1970

      boost::gregorian::date lowDate(boost::gregorian::from_string("1960/01/01"));  // low date
      boost::gregorian::date highDate(boost::gregorian::from_string("1970/01/01")); // high date

      OksAndExpression * andExpression = new OksAndExpression();

      andExpression->add(new OksComparator(PersonBirthday, new OksData(lowDate), OksQuery::greater_cmp));
      andExpression->add(new OksComparator(PersonBirthday, new OksData(highDate), OksQuery::less_cmp));

      OksQuery query(true, andExpression);


        // executes query

      std::cout << "[QUERY]: Looking for persons were born between " << lowDate
                << " and " << highDate << " ...\n\n";
        
      OksObject::List * queryResult = Person->execute_query(&query);


        // builds iterator over results if something was found

     if(queryResult) {
        std::cout << "[QUERY]: Query \'" << query
                  << "\'\n  founds the following objects in class \'"
                  << Person->get_name() << "\' and subclasses:\n";

        for(OksObject::List::iterator i = queryResult->begin(); i != queryResult->end(); ++i) {
          OksObject * o = *i;
          OksData * d(o->GetAttributeValue("Birthday"));
          std::cout << "   - " << o << " was born " << *d << std::endl;
        }


          // free list: we do not need it anymore

        delete queryResult;
      }

      std::cout << "[QUERY]: Done database querying with logical function\n\n";

    }
  
    {

      std::cout << "[QUERY]: Start database querying with relationship expression...\n";


        // Looking for employee were born after 01 January 1971
        // and which works at IT Department

      boost::gregorian::date aDate(boost::gregorian::from_string("1971/01/01"));  // a date
      const char * departmentName = "IT Department"; 

      OksAndExpression * andExpression = new OksAndExpression();
  
      andExpression->add(new OksComparator(PersonBirthday, new OksData(aDate), OksQuery::greater_cmp));
      andExpression->add(new OksRelationshipExpression(WorksAt, new OksComparator(DepartmentName, new OksData(departmentName), OksQuery::equal_cmp)));
                
      OksQuery query(true, andExpression);


        // executes query

      std::cout << "[QUERY]: Looking for employee were born after " << aDate
                << " and works at department " << departmentName << " ...\n\n";

      OksObject::List * queryResult = Employee->execute_query(&query);


        // builds iterator over results if something was found

     if(queryResult) {
        std::cout << "[QUERY]: Query \'" << query
                  << "\'\n  founds the following objects in class \'"
                  << Employee->get_name() << "\' and subclasses:\n";

        for(OksObject::List::iterator i = queryResult->begin(); i != queryResult->end(); ++i) {
          OksObject * o = *i;
          OksData * d(o->GetAttributeValue("Birthday"));
          std::cout << "   - " << o << " was born " << *d << std::endl;
        }


          // free list: we do not need it anymore

        delete queryResult;
      }
  
      std::cout << "[QUERY]: Done database querying with relationship expression...\n";
    }

    std::cout << "\n[OKS TUTORIAL]: Done with database querying tests\n\n";

      // It is not necessary to free data or schema because all instances of OKS
      // classes are automatic C++ objects and they will deleted before last
      // close bracket `}`.

  }
  catch (const std::exception & ex) {
    std::cerr << "Caught exception:\n" << ex.what() << std::endl;
  }

    // returns success

  return 0;
}
