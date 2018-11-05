import java.lang.reflect.Method; 

class MethodReflection {

    public static void main(String[] args) throws Exception {
        MethodReflection mr = new MethodReflection();
        Class cls = mr.getClass();
        Method[] m = cls.getDeclaredMethods();
        for (Method mtd : m) {
            if (mtd.getName().equals("<init>")) continue;
            System.out.println(mtd.getName());
            System.out.println(mtd.getModifiers());

            if (mtd.getName().equals("ll")) {
                System.out.println(mtd.invoke(mr, "aa"));
            }
        }
    }

    public String ll(String s) {
        return "Called LL with "+s;
    }

}