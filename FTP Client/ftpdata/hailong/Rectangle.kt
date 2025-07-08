import java.util.Scanner

class Rectangle
{
    var length: Double = 0.0
        get ()
        {
            return field
        }
        
        set (length)
        {
            field = length
        }
    
    var width: Double = 0.0
        get ()
        {
            return field
        }
        
        set (width)
        {
            field = width
        }
    
    val area: Double get ()
    {
        return length * width
    }
    
    val parameter: Double get ()
    {
        return 2 * (length + width)
    }
}

fun main ()
{
    val s = Scanner (System.`in`)
    val r = Rectangle ()
    
    r.length = s.nextDouble ()
    r.width = s.nextDouble ()

    println ("Area: ${r.length} * ${r.width} = ${r.area}")
    println ("Parameter: 2 * (${r.length} + ${r.width}) = ${r.parameter}")
    
    s.close ()
}